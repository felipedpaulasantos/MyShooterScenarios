// Copyright MyShooterScenarios. All Rights Reserved.

#include "AI/BTService_AIStateObserver.h"

#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/LyraHealthComponent.h"
#include "GameFramework/Pawn.h"

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

UBTService_AIStateObserver::UBTService_AIStateObserver(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = "AI State Observer (MYST)";
	bNotifyTick = true;

	// Each AI controller gets its own UObject instance of this service so we
	// can safely use member variables for per-AI state (LastDamageTime, etc.)
	// without any manual node-memory management.
	bCreateNodeInstance = true;

	Interval = 0.15f;
	RandomDeviation = 0.05f;

	ReloadTag = FGameplayTag::RequestGameplayTag(FName("Event.Movement.Reload"), false);

	OutOfAmmoKey.AddBoolFilter(this,             GET_MEMBER_NAME_CHECKED(UBTService_AIStateObserver, OutOfAmmoKey));
	HasTakenDamageRecentlyKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_AIStateObserver, HasTakenDamageRecentlyKey));
	TargetEnemyKey.AddObjectFilter(this,          GET_MEMBER_NAME_CHECKED(UBTService_AIStateObserver, TargetEnemyKey), AActor::StaticClass());
	TargetIsReloadingKey.AddBoolFilter(this,      GET_MEMBER_NAME_CHECKED(UBTService_AIStateObserver, TargetIsReloadingKey));
	TargetIsLowHealthKey.AddBoolFilter(this,      GET_MEMBER_NAME_CHECKED(UBTService_AIStateObserver, TargetIsLowHealthKey));
}

// ─────────────────────────────────────────────────────────────────────────────
// Asset initialisation
// ─────────────────────────────────────────────────────────────────────────────

void UBTService_AIStateObserver::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		OutOfAmmoKey.ResolveSelectedKey(*BBAsset);
		HasTakenDamageRecentlyKey.ResolveSelectedKey(*BBAsset);
		TargetEnemyKey.ResolveSelectedKey(*BBAsset);
		TargetIsReloadingKey.ResolveSelectedKey(*BBAsset);
		TargetIsLowHealthKey.ResolveSelectedKey(*BBAsset);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Become / Cease Relevant
// ─────────────────────────────────────────────────────────────────────────────

void UBTService_AIStateObserver::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	// Reset per-activation state.
	LastDamageTime = -1.f;
	bWasReloading  = false;
	OwnerBTComp    = &OwnerComp;

	// Sync the BB key to the freshly-reset state so no stale true lingers
	// from a previous activation cycle.
	if (HasTakenDamageRecentlyKey.IsSet())
	{
		if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
		{
			BB->SetValueAsBool(HasTakenDamageRecentlyKey.SelectedKeyName, false);
		}
	}

	// Attempt an early bind — may fail if the pawn isn't possessed yet,
	// in which case the lazy bind in TickNode will retry.
	if (AAIController* AIC = OwnerComp.GetAIOwner())
	{
		if (APawn* Pawn = AIC->GetPawn())
		{
			if (ULyraHealthComponent* HealthComp = ULyraHealthComponent::FindHealthComponent(Pawn))
			{
				TrackedHealthComp = HealthComp;
				HealthComp->OnHealthChanged.AddDynamic(this, &UBTService_AIStateObserver::OnAIHealthChanged);
			}
		}
	}
}

void UBTService_AIStateObserver::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UnbindHealthDelegate();
	OwnerBTComp = nullptr;

	Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}

// ─────────────────────────────────────────────────────────────────────────────
// Health delegate callback  (fires immediately when damage lands)
// ─────────────────────────────────────────────────────────────────────────────

void UBTService_AIStateObserver::OnAIHealthChanged(
	ULyraHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* /*Instigator*/)
{
	if (NewValue >= OldValue) { return; }

	const float MaxHealth = HealthComponent->GetMaxHealth();
	if (MaxHealth <= 0.f) { return; }

	const float DamageFraction = (OldValue - NewValue) / MaxHealth;
	if (DamageFraction < DamageReactionThreshold) { return; }

	if (UWorld* World = GetWorld())
	{
		LastDamageTime = World->GetTimeSeconds();
	}

	if (UBehaviorTreeComponent* BTComp = OwnerBTComp.Get())
	{
		if (HasTakenDamageRecentlyKey.IsSet())
		{
			if (UBlackboardComponent* BB = BTComp->GetBlackboardComponent())
			{
				BB->SetValueAsBool(HasTakenDamageRecentlyKey.SelectedKeyName, true);
			}
		}
	}

	OnDamageDetected(DamageFraction);
}

// ─────────────────────────────────────────────────────────────────────────────
// Helper
// ─────────────────────────────────────────────────────────────────────────────

void UBTService_AIStateObserver::UnbindHealthDelegate()
{
	if (ULyraHealthComponent* HealthComp = TrackedHealthComp.Get())
	{
		HealthComp->OnHealthChanged.RemoveDynamic(this, &UBTService_AIStateObserver::OnAIHealthChanged);
	}
	TrackedHealthComp = nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// Tick
// ─────────────────────────────────────────────────────────────────────────────

void UBTService_AIStateObserver::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	// Throttled heartbeat removed — re-add if debugging is needed.

	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

	if (!AIController || !Blackboard)
	{
		return;
	}

	APawn* AIPawn = AIController->GetPawn();
	if (!AIPawn)
	{
		return;
	}

	// ── Lazy bind ────────────────────────────────────────────────────────────
	// OnBecomeRelevant may fire before the pawn is possessed (or before its
	// health component is ready), leaving TrackedHealthComp null.  Retry on
	// every tick until we succeed.
	// OwnerBTComp is also set here as a fallback for the same reason.
	if (!OwnerBTComp.IsValid())
	{
		OwnerBTComp = &OwnerComp;
	}

	if (!TrackedHealthComp.IsValid())
	{
		if (ULyraHealthComponent* HealthComp = ULyraHealthComponent::FindHealthComponent(AIPawn))
		{
			TrackedHealthComp = HealthComp;
			HealthComp->OnHealthChanged.AddDynamic(this, &UBTService_AIStateObserver::OnAIHealthChanged);
		}
	}

	// ─── 1. AI reload state → OutOfAmmoKey ───────────────────────────────────
	if (OutOfAmmoKey.IsSet() && ReloadTag.IsValid())
	{
		if (UAbilitySystemComponent* AIASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AIPawn))
		{
			const bool bIsReloading = AIASC->HasMatchingGameplayTag(ReloadTag);
			Blackboard->SetValueAsBool(OutOfAmmoKey.SelectedKeyName, bIsReloading);

			if (bIsReloading != bWasReloading)
			{
				OnAIReloadStateChanged(bIsReloading);
				bWasReloading = bIsReloading;
			}
		}
	}

	// ─── 2. HasTakenDamageRecently cooldown expiry ────────────────────────────
	//
	// Damage arming happens instantly in OnAIHealthChanged (delegate-driven).
	// This tick only needs to check whether the cooldown window has elapsed
	// and clear the flag if so.
	if (HasTakenDamageRecentlyKey.IsSet() && LastDamageTime >= 0.f)
	{
		const float Elapsed = GetWorld()->GetTimeSeconds() - LastDamageTime;
		if (Elapsed >= DamageCooldown)
		{
			LastDamageTime = -1.f;
			Blackboard->SetValueAsBool(HasTakenDamageRecentlyKey.SelectedKeyName, false);
		}
	}

	// ─── 3 & 4. Target-related keys ─────────────────────────────────────────
	const bool bNeedTargetKeys = TargetIsReloadingKey.IsSet() || TargetIsLowHealthKey.IsSet();
	if (bNeedTargetKeys && TargetEnemyKey.IsSet())
	{
		AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(TargetEnemyKey.SelectedKeyName));

		if (TargetIsReloadingKey.IsSet())
		{
			bool bTargetReloading = false;
			if (TargetActor && ReloadTag.IsValid())
			{
				if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor))
				{
					bTargetReloading = TargetASC->HasMatchingGameplayTag(ReloadTag);
				}
			}
			Blackboard->SetValueAsBool(TargetIsReloadingKey.SelectedKeyName, bTargetReloading);
		}

		if (TargetIsLowHealthKey.IsSet())
		{
			bool bTargetLowHealth = false;
			if (TargetActor)
			{
				if (ULyraHealthComponent* TargetHealth = ULyraHealthComponent::FindHealthComponent(TargetActor))
				{
					bTargetLowHealth = TargetHealth->GetHealthNormalized() < LowHealthThreshold;
				}
			}
			Blackboard->SetValueAsBool(TargetIsLowHealthKey.SelectedKeyName, bTargetLowHealth);
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Description
// ─────────────────────────────────────────────────────────────────────────────

FString UBTService_AIStateObserver::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("Reload tag: %s\nDamage cooldown: %.1fs  |  Hit threshold: %.0f%%\nLow health threshold: %.0f%%"),
		ReloadTag.IsValid() ? *ReloadTag.ToString() : TEXT("(none set)"),
		DamageCooldown,
		DamageReactionThreshold * 100.f,
		LowHealthThreshold * 100.f);
}

