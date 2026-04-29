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

	// Poll frequently enough for responsive damage / reload detection.
	// Designers can raise this on cheaper enemy types.
	Interval = 0.15f;
	RandomDeviation = 0.05f;

	// Default reload tag — must match ActivationOwnedTags on the reload GA.
	// Resolves at construction; safe because tag tables are registered before gameplay.
	ReloadTag = FGameplayTag::RequestGameplayTag(FName("Event.Movement.Reload"), /*bErrorIfNotFound=*/false);

	// Restrict each key selector to its expected type so the editor only shows valid keys
	// and — critically — so IsSet() returns true once ResolveSelectedKey has been called.
	OutOfAmmoKey.AddBoolFilter(this,              GET_MEMBER_NAME_CHECKED(UBTService_AIStateObserver, OutOfAmmoKey));
	HasTakenDamageRecentlyKey.AddBoolFilter(this,  GET_MEMBER_NAME_CHECKED(UBTService_AIStateObserver, HasTakenDamageRecentlyKey));
	TargetEnemyKey.AddObjectFilter(this,           GET_MEMBER_NAME_CHECKED(UBTService_AIStateObserver, TargetEnemyKey), AActor::StaticClass());
	TargetIsReloadingKey.AddBoolFilter(this,       GET_MEMBER_NAME_CHECKED(UBTService_AIStateObserver, TargetIsReloadingKey));
	TargetIsLowHealthKey.AddBoolFilter(this,       GET_MEMBER_NAME_CHECKED(UBTService_AIStateObserver, TargetIsLowHealthKey));
}

// ─────────────────────────────────────────────────────────────────────────────
// Asset initialisation — resolves SelectedKeyID for every key selector
// ─────────────────────────────────────────────────────────────────────────────

void UBTService_AIStateObserver::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	// ResolveSelectedKey populates SelectedKeyID from SelectedKeyName against
	// the Blackboard asset. Without this, IsSet() always returns false and
	// TickNode silently skips every key write.
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
// Diagnostic: log key bindings on activation so mismapped keys are obvious
// ─────────────────────────────────────────────────────────────────────────────

void UBTService_AIStateObserver::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	UE_LOG(LogTemp, Warning,
		TEXT("[AIStateObserver] Key map — OutOfAmmo:'%s'  HasTakenDamageRecently:'%s'"
		     "  TargetEnemy:'%s'  TargetIsReloading:'%s'  TargetIsLowHealth:'%s'"),
		*OutOfAmmoKey.SelectedKeyName.ToString(),
		*HasTakenDamageRecentlyKey.SelectedKeyName.ToString(),
		*TargetEnemyKey.SelectedKeyName.ToString(),
		*TargetIsReloadingKey.SelectedKeyName.ToString(),
		*TargetIsLowHealthKey.SelectedKeyName.ToString()
	);
}

// ─────────────────────────────────────────────────────────────────────────────
// Instance memory
// ─────────────────────────────────────────────────────────────────────────────

uint16 UBTService_AIStateObserver::GetInstanceMemorySize() const
{
	return sizeof(FAIStateObserverMemory);
}

void UBTService_AIStateObserver::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	new (NodeMemory) FAIStateObserverMemory();
}

// ─────────────────────────────────────────────────────────────────────────────
// Tick
// ─────────────────────────────────────────────────────────────────────────────

void UBTService_AIStateObserver::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	FAIStateObserverMemory* Memory = reinterpret_cast<FAIStateObserverMemory*>(NodeMemory);

	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

	if (!Memory || !AIController || !Blackboard)
	{
		return;
	}

	APawn* AIPawn = AIController->GetPawn();
	if (!AIPawn)
	{
		return;
	}

	// ─── 1. AI reload state → OutOfAmmoKey ───────────────────────────────────
	if (OutOfAmmoKey.IsSet() && ReloadTag.IsValid())
	{
		if (UAbilitySystemComponent* AIASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AIPawn))
		{
			const bool bIsReloading = AIASC->HasMatchingGameplayTag(ReloadTag);
			Blackboard->SetValueAsBool(OutOfAmmoKey.SelectedKeyName, bIsReloading);

			// Fire Blueprint transition event
			if (bIsReloading != Memory->bWasReloading)
			{
				OnAIReloadStateChanged(bIsReloading);
				Memory->bWasReloading = bIsReloading;
			}
		}
	}

	// ─── 2. Damage detection → HasTakenDamageRecentlyKey ────────────────────
	//
	// Strategy: compare current health to the value cached last tick.
	// A drop ≥ DamageReactionThreshold arms the flag and resets the cooldown.
	// The cooldown counts up via TimeSinceLastHit; once it exceeds DamageCooldown
	// the flag is cleared.  Everything lives in instance memory — no delegates.
	//
	// bInitialized guard: after a BT abort the service memory is re-created with
	// LastKnownHealthPct = 1.f while the pawn may already be at lower health,
	// which would produce a spurious large delta on the very first tick and
	// immediately re-arm the flag.  We skip the comparison on that first tick.
	if (HasTakenDamageRecentlyKey.IsSet())
	{
		if (ULyraHealthComponent* HealthComp = ULyraHealthComponent::FindHealthComponent(AIPawn))
		{
			const float CurrentHealthPct = HealthComp->GetHealthNormalized();

			if (!Memory->bInitialized)
			{
				// First tick after (re)initialization — just snapshot health, no comparison.
				Memory->LastKnownHealthPct = CurrentHealthPct;
				Memory->bInitialized = true;
			}
			else
			{
				const float HealthDelta = Memory->LastKnownHealthPct - CurrentHealthPct;

				if (HealthDelta >= DamageReactionThreshold)
				{
					// Significant hit this tick — arm the flag and reset the timer.
					Memory->TimeSinceLastHit = 0.f;
					Memory->bDamageArmed = true;
					Blackboard->SetValueAsBool(HasTakenDamageRecentlyKey.SelectedKeyName, true);
					OnDamageDetected(HealthDelta);
				}
				else if (Memory->bDamageArmed)
				{
					// Flag is armed — advance the cooldown timer using memory, not the BB.
					Memory->TimeSinceLastHit += DeltaSeconds;
					if (Memory->TimeSinceLastHit >= DamageCooldown)
					{
						Memory->bDamageArmed = false;
						Blackboard->SetValueAsBool(HasTakenDamageRecentlyKey.SelectedKeyName, false);
					}
				}

				Memory->LastKnownHealthPct = CurrentHealthPct;
			}
		}
	}

	// ─── 3 & 4. Target-related keys ─────────────────────────────────────────
	//
	// Resolve the target actor once and reuse it for both checks below.
	// GetAbilitySystemComponentFromActor handles the player-owns-ASC-on-PlayerState
	// case transparently (via IAbilitySystemInterface on ALyraPlayerState).
	const bool bNeedTargetKeys = TargetIsReloadingKey.IsSet() || TargetIsLowHealthKey.IsSet();
	if (bNeedTargetKeys && TargetEnemyKey.IsSet())
	{
		AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(TargetEnemyKey.SelectedKeyName));

		// ── 3. Target reloading → TargetIsReloadingKey ───────────────────────
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

		// ── 4. Target low health → TargetIsLowHealthKey ──────────────────────
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
// Description (shown in the BT node header in the editor)
// ─────────────────────────────────────────────────────────────────────────────

FString UBTService_AIStateObserver::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("Reload tag: %s\nDamage cooldown: %.1fs  |  Hit threshold: %.0f%%\nLow health threshold: %.0f%%"),
		ReloadTag.IsValid() ? *ReloadTag.ToString() : TEXT("(none set)"),
		DamageCooldown,
		DamageReactionThreshold * 100.f,
		LowHealthThreshold * 100.f
	);
}

