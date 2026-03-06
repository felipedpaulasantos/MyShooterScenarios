// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/LyraBlockHealthLogicComponent.h"

#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "Character/LyraHealthComponent.h"

#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "Logging/LogMacros.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraBlockHealthLogicComponent)

DEFINE_LOG_CATEGORY_STATIC(LogLyraBlockHealthLogic, Log, All);

ULyraBlockHealthLogicComponent::ULyraBlockHealthLogicComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULyraBlockHealthLogicComponent::BeginPlay()
{
	Super::BeginPlay();

	TryBindToHealthComponent();

	if (bQuantizeOnBeginPlay)
	{
		QuantizeNow();
	}
}

void ULyraBlockHealthLogicComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.RemoveDynamic(this, &ThisClass::HandleHealthChanged);
		HealthComponent->OnMaxHealthChanged.RemoveDynamic(this, &ThisClass::HandleMaxHealthChanged);
	}

	HealthComponent = nullptr;

	Super::EndPlay(EndPlayReason);
}

bool ULyraBlockHealthLogicComponent::IsServerAuthority() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}

void ULyraBlockHealthLogicComponent::QuantizeNow()
{
	if (!bEnableBlockHealth)
	{
		return;
	}

	if (!IsServerAuthority())
	{
		return;
	}

	if (!HealthComponent)
	{
		TryBindToHealthComponent();
	}

	if (!HealthComponent)
	{
		return;
	}

	// Force a health "change" into the quantizer. Use current health for both values.
	ApplyQuantizedHealthIfNeeded(HealthComponent->GetHealth(), HealthComponent->GetHealth());
}

void ULyraBlockHealthLogicComponent::TryBindToHealthComponent()
{
	if (HealthComponent)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogLyraBlockHealthLogic, Verbose, TEXT("TryBindToHealthComponent: No owner"));
		return;
	}

	HealthComponent = Owner->FindComponentByClass<ULyraHealthComponent>();
	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.AddDynamic(this, &ThisClass::HandleHealthChanged);
		HealthComponent->OnMaxHealthChanged.AddDynamic(this, &ThisClass::HandleMaxHealthChanged);

		// Initial broadcast for UI.
		const int32 Blocks = GetCurrentBlocks_Implementation();
		OnBlocksChanged.Broadcast(this, Blocks, Blocks, nullptr);
	}
	else
	{
		UE_LOG(LogLyraBlockHealthLogic, Verbose, TEXT("TryBindToHealthComponent: '%s' has no ULyraHealthComponent (blocks will read as 0)"), *GetNameSafe(Owner));
	}
}

ULyraAbilitySystemComponent* ULyraBlockHealthLogicComponent::GetLyraASC() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner);
	if (!ASI)
	{
		return nullptr;
	}

	return Cast<ULyraAbilitySystemComponent>(ASI->GetAbilitySystemComponent());
}

int32 ULyraBlockHealthLogicComponent::HealthToBlocks_RoundUp(float Health) const
{
	if (BlockSize <= KINDA_SMALL_NUMBER)
	{
		return 0;
	}

	return FMath::CeilToInt(FMath::Max(0.0f, Health) / BlockSize);
}

float ULyraBlockHealthLogicComponent::BlocksToHealth(int32 Blocks) const
{
	if (BlockSize <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	return FMath::Max(0, Blocks) * BlockSize;
}

int32 ULyraBlockHealthLogicComponent::GetCurrentBlocks_Implementation() const
{
	// Lazily bind in case component init order meant BeginPlay couldn't find the health component yet.
	if (!HealthComponent)
	{
		const_cast<ULyraBlockHealthLogicComponent*>(this)->TryBindToHealthComponent();
	}

	return HealthComponent ? HealthToBlocks_RoundUp(HealthComponent->GetHealth()) : 0;
}

int32 ULyraBlockHealthLogicComponent::GetMaxBlocks_Implementation() const
{
	// Lazily bind in case component init order meant BeginPlay couldn't find the health component yet.
	if (!HealthComponent)
	{
		const_cast<ULyraBlockHealthLogicComponent*>(this)->TryBindToHealthComponent();
	}

	return HealthComponent ? HealthToBlocks_RoundUp(HealthComponent->GetMaxHealth()) : 0;
}

float ULyraBlockHealthLogicComponent::GetBlocksNormalized_Implementation() const
{
	const int32 MaxBlocks = GetMaxBlocks_Implementation();
	return (MaxBlocks > 0) ? (static_cast<float>(GetCurrentBlocks_Implementation()) / static_cast<float>(MaxBlocks)) : 0.0f;
}

void ULyraBlockHealthLogicComponent::HandleMaxHealthChanged(ULyraHealthComponent* InHealthComponent, float OldValue, float NewValue, AActor* Instigator)
{
	if (!bEnableBlockHealth || !InHealthComponent)
	{
		return;
	}

	const int32 OldMaxBlocks = HealthToBlocks_RoundUp(OldValue);
	const int32 NewMaxBlocks = HealthToBlocks_RoundUp(NewValue);

	if (OldMaxBlocks != NewMaxBlocks)
	{
		OnBlocksChanged.Broadcast(this, OldMaxBlocks, NewMaxBlocks, Instigator);
	}
}

void ULyraBlockHealthLogicComponent::HandleHealthChanged(ULyraHealthComponent* InHealthComponent, float OldValue, float NewValue, AActor* Instigator)
{
	if (!bEnableBlockHealth || !InHealthComponent)
	{
		return;
	}

	const int32 OldBlocks = HealthToBlocks_RoundUp(OldValue);

	ApplyQuantizedHealthIfNeeded(OldValue, NewValue);

	const float FinalHealth = InHealthComponent->GetHealth();
	const int32 NewBlocks = HealthToBlocks_RoundUp(FinalHealth);

	if (OldBlocks != NewBlocks)
	{
		OnBlocksChanged.Broadcast(this, OldBlocks, NewBlocks, Instigator);
	}
}

void ULyraBlockHealthLogicComponent::ApplyQuantizedHealthIfNeeded(float OldHealth, float NewHealth)
{
	if (bApplyingQuantization)
	{
		return;
	}

	if (!bEnableBlockHealth)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	if (!HealthComponent)
	{
		return;
	}

	ULyraAbilitySystemComponent* ASC = GetLyraASC();
	if (!ASC)
	{
		return;
	}

	if (BlockSize <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// No delta -> nothing to do.
	if (FMath::IsNearlyEqual(OldHealth, NewHealth))
	{
		return;
	}

	const float MaxHealth = HealthComponent->GetMaxHealth();
	const float ClampedOld = FMath::Clamp(OldHealth, 0.0f, MaxHealth);
	const float ClampedNew = FMath::Clamp(NewHealth, 0.0f, MaxHealth);

	const bool bIsDamage = (ClampedNew < ClampedOld);
	const bool bIsHealing = (ClampedNew > ClampedOld);

	if (!bIsDamage && !bIsHealing)
	{
		return;
	}

	float QuantizedTargetHealth;

	if (bIsDamage)
	{
		const float DamageAmount = ClampedOld - ClampedNew;
		const int32 DamageBlocks = (DamageAmount <= TwoBlockDamageThreshold) ? 1 : 2;
		const int32 ClampedDamageBlocks = FMath::Clamp(DamageBlocks, 1, FMath::Max(1, MaxDamageBlocksPerHit));

		const int32 OldBlocks = HealthToBlocks_RoundUp(ClampedOld);
		const int32 NewBlocks = FMath::Max(0, OldBlocks - ClampedDamageBlocks);
		QuantizedTargetHealth = FMath::Clamp(BlocksToHealth(NewBlocks), 0.0f, MaxHealth);
	}
	else
	{
		const float HealAmount = ClampedNew - ClampedOld;
		const int32 HealBlocks = FMath::Max(1, FMath::CeilToInt(HealAmount / BlockSize));

		const int32 OldBlocks = HealthToBlocks_RoundUp(ClampedOld);
		const int32 MaxBlocks = GetMaxBlocks_Implementation();
		const int32 NewBlocks = FMath::Min(MaxBlocks, OldBlocks + HealBlocks);
		QuantizedTargetHealth = FMath::Clamp(BlocksToHealth(NewBlocks), 0.0f, MaxHealth);
	}

	if (FMath::IsNearlyEqual(QuantizedTargetHealth, ClampedNew, 0.01f))
	{
		return;
	}

	// IMPORTANT: Lyra death flow is triggered by ULyraHealthSet::OnOutOfHealth, which is broadcast
	// from ULyraHealthSet::PostGameplayEffectExecute (i.e. when damage/heal meta attributes are applied).
	// Directly overriding Health via SetNumericAttributeBase can bypass that flow, leaving the pawn "alive"
	// even if Health visually hits 0.
	//
	// So, when our quantization would drive health to 0, route it through the standard self-destruct damage GE.
	if (QuantizedTargetHealth <= 0.0f)
	{
		// Avoid loops: DamageSelfDestruct applies damage, which will re-enter health change delegates.
		bApplyingQuantization = true;
		UE_LOG(LogLyraBlockHealthLogic, Verbose, TEXT("ApplyQuantizedHealthIfNeeded: quantized to 0 -> triggering Lyra death flow via DamageSelfDestruct for '%s'"), *GetNameSafe(Owner));
		HealthComponent->DamageSelfDestruct(/*bFellOutOfWorld=*/ false);
		bApplyingQuantization = false;
		return;
	}

	bApplyingQuantization = true;
	ASC->SetNumericAttributeBase(ULyraHealthSet::GetHealthAttribute(), QuantizedTargetHealth);
	bApplyingQuantization = false;
}
