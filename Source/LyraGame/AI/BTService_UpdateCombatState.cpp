// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/BTService_UpdateCombatState.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/LyraHealthComponent.h"
#include "Teams/LyraTeamSubsystem.h"
#include "EngineUtils.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

UBTService_UpdateCombatState::UBTService_UpdateCombatState()
{
	NodeName = "Update Combat State";
	bNotifyTick = true;
	IsolationRadius = 1500.0f;
	HealthWriteDeadBand = 0.01f;
	IsolationHysteresis = 100.f;

	// Restrict each key selector to its expected type so the editor dropdown only shows valid keys.
	HealthPercentageKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCombatState, HealthPercentageKey));
	IsIsolatedKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCombatState, IsIsolatedKey));
	HasTargetInCoverKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCombatState, HasTargetInCoverKey));
	TargetEnemyKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCombatState, TargetEnemyKey), AActor::StaticClass());
	TargetTimeInCoverKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCombatState, TargetTimeInCoverKey));
	IsTargetEngagingOtherKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCombatState, IsTargetEngagingOtherKey));
}

void UBTService_UpdateCombatState::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	// ResolveSelectedKey populates SelectedKeyID from SelectedKeyName against the BB asset.
	// Without this, IsSet() always returns false and TickNode writes nothing.
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		HealthPercentageKey.ResolveSelectedKey(*BBAsset);
		IsIsolatedKey.ResolveSelectedKey(*BBAsset);
		HasTargetInCoverKey.ResolveSelectedKey(*BBAsset);
		TargetEnemyKey.ResolveSelectedKey(*BBAsset);
		TargetTimeInCoverKey.ResolveSelectedKey(*BBAsset);
		IsTargetEngagingOtherKey.ResolveSelectedKey(*BBAsset);
	}
}

void UBTService_UpdateCombatState::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	// ── Diagnostic: log every key selector name so a mismatched BT-editor
	// assignment (e.g. a combat key wired to HasSeenPlayer) is immediately visible.
	UE_LOG(LogTemp, Warning,
		TEXT("[UpdateCombatState] Key map — HealthPct:'%s'  IsIsolated:'%s'  HasTargetInCover:'%s'"
			 "  TargetEnemy:'%s'  TargetTimeInCover:'%s'  IsTargetEngagingOther:'%s'"),
		*HealthPercentageKey.SelectedKeyName.ToString(),
		*IsIsolatedKey.SelectedKeyName.ToString(),
		*HasTargetInCoverKey.SelectedKeyName.ToString(),
		*TargetEnemyKey.SelectedKeyName.ToString(),
		*TargetTimeInCoverKey.SelectedKeyName.ToString(),
		*IsTargetEngagingOtherKey.SelectedKeyName.ToString()
	);
}

uint16 UBTService_UpdateCombatState::GetInstanceMemorySize() const
{
	return sizeof(FCombatStateMemory);
}

void UBTService_UpdateCombatState::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	new (NodeMemory) FCombatStateMemory();
}

void UBTService_UpdateCombatState::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	FCombatStateMemory* MyMemory = reinterpret_cast<FCombatStateMemory*>(NodeMemory);

	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!AIController || !BlackboardComp)
	{
		return;
	}

	APawn* ControlledPawn = AIController->GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	// ── Health Percentage ─────────────────────────────────────────────────────
	//
	// Dead-band guard: only write to the BB when health changes by more than
	// HealthWriteDeadBand.  Without this, passive regen ticks and floating-point
	// precision drift update the key every tick, firing BB change notifications
	// that trigger Observer-Abort decorators and cause child tasks to be aborted.
	if (HealthPercentageKey.IsSet() && MyMemory)
	{
		if (ULyraHealthComponent* HealthComp = ULyraHealthComponent::FindHealthComponent(ControlledPawn))
		{
			const float NewPct = HealthComp->GetHealthNormalized();
			if (FMath::Abs(NewPct - MyMemory->LastWrittenHealthPct) >= HealthWriteDeadBand)
			{
				MyMemory->LastWrittenHealthPct = NewPct;
				BlackboardComp->SetValueAsFloat(HealthPercentageKey.SelectedKeyName, NewPct);
			}
		}
	}

	// ── Is Isolated ───────────────────────────────────────────────────────────
	//
	// Hysteresis guard: an ally must move within (IsolationRadius - IsolationHysteresis)
	// to clear the flag, but must leave beyond IsolationRadius to set it.
	// Without this, an ally standing exactly on the boundary flips the bool every
	// tick and fires BB notifications at the service's full tick rate.
	if (IsIsolatedKey.IsSet() && MyMemory && ControlledPawn->GetWorld())
	{
		if (ULyraTeamSubsystem* TeamSubsystem = ControlledPawn->GetWorld()->GetSubsystem<ULyraTeamSubsystem>())
		{
			bool bIsPartOfTeam = false;
			int32 MyTeamId = INDEX_NONE;
			TeamSubsystem->FindTeamFromActor(ControlledPawn, bIsPartOfTeam, MyTeamId);

			// Seed from cached state (hysteresis: different thresholds to enter vs exit).
			bool bIsIsolated = MyMemory->bCachedIsIsolated;

			if (bIsPartOfTeam)
			{
				const FVector MyLocation = ControlledPawn->GetActorLocation();

				// Use a tighter inner radius to CLEAR isolation, standard radius to SET it.
				const float CheckRadius = bIsIsolated
					? (IsolationRadius - IsolationHysteresis)
					: IsolationRadius;
				const float RadiusSq = CheckRadius * CheckRadius;

				bool bFoundAlly = false;
				for (TActorIterator<APawn> It(ControlledPawn->GetWorld()); It; ++It)
				{
					APawn* OtherPawn = *It;
					if (OtherPawn != ControlledPawn && !OtherPawn->IsPlayerControlled())
					{
						bool bOtherInTeam = false;
						int32 OtherTeamId = INDEX_NONE;
						TeamSubsystem->FindTeamFromActor(OtherPawn, bOtherInTeam, OtherTeamId);

						if (bOtherInTeam && OtherTeamId == MyTeamId)
						{
							if (FVector::DistSquared(MyLocation, OtherPawn->GetActorLocation()) <= RadiusSq)
							{
								bFoundAlly = true;
								break;
							}
						}
					}
				}

				bIsIsolated = !bFoundAlly;
			}
			else
			{
				bIsIsolated = true;
			}

			// Only write to BB when the value actually flips.
			if (bIsIsolated != MyMemory->bCachedIsIsolated)
			{
				MyMemory->bCachedIsIsolated = bIsIsolated;
				BlackboardComp->SetValueAsBool(IsIsolatedKey.SelectedKeyName, bIsIsolated);
			}
		}
	}

	// ── Target-in-Cover + Engagement tracking ────────────────────────────────
	if (TargetEnemyKey.IsSet() && HasTargetInCoverKey.IsSet())
	{
		AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetEnemyKey.SelectedKeyName));
		bool bTargetInCover = false;

		if (TargetActor)
		{
			if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor))
			{
				// In Lyra, entering cover grants a GameplayTag (e.g. "State.Cover").
				static FGameplayTag CoverTag = FGameplayTag::RequestGameplayTag(FName("State.Cover"), false);
				if (CoverTag.IsValid() && TargetASC->HasMatchingGameplayTag(CoverTag))
				{
					bTargetInCover = true;
				}
			}
		}

		BlackboardComp->SetValueAsBool(HasTargetInCoverKey.SelectedKeyName, bTargetInCover);

		// ── IsTargetEngagingOther — hysteresis-guarded dot-product check ─────────
		//
		// Problem: GetActorForwardVector() changes every tick due to character
		// animation and locomotion rotation. A hard threshold (e.g. 0.5) means
		// the value oscillates true/false at tick-rate whenever the target faces
		// near that angle, triggering Observer-Abort decorators and restarting
		// the BT every frame (the "blinking" symptom).
		//
		// Fix: use separate ENTER (< 0.30) and EXIT (>= 0.65) thresholds and only
		// commit the value to the blackboard when it actually changes. The dead-band
		// between 0.30 and 0.65 absorbs normal frame-to-frame noise.
		if (IsTargetEngagingOtherKey.IsSet() && MyMemory)
		{
			// Seed from cached state (hysteresis: sticky until a threshold is crossed).
			bool bEngagingOther = MyMemory->bCachedEngagingOther;

			if (TargetActor)
			{
				const FVector TargetForward = TargetActor->GetActorForwardVector();
				const FVector TargetLoc     = TargetActor->GetActorLocation();
				const FVector MyLoc         = ControlledPawn->GetActorLocation();
				const float   DotToMe       = FVector::DotProduct(TargetForward, (MyLoc - TargetLoc).GetSafeNormal());

				if (!bEngagingOther)
				{
					// SET true only when target is clearly NOT facing us (< 0.30 ≈ 72°).
					if (DotToMe < 0.30f)
					{
						if (ULyraTeamSubsystem* TeamSubsystem = ControlledPawn->GetWorld()->GetSubsystem<ULyraTeamSubsystem>())
						{
							const int32 MyTeamId = TeamSubsystem->FindTeamFromObject(ControlledPawn);
							if (MyTeamId != INDEX_NONE)
							{
								for (TActorIterator<APawn> It(ControlledPawn->GetWorld()); It; ++It)
								{
									APawn* OtherPawn = *It;
									if (OtherPawn && OtherPawn != ControlledPawn && OtherPawn != TargetActor)
									{
										if (TeamSubsystem->FindTeamFromObject(OtherPawn) == MyTeamId)
										{
											const FVector TargetToAlly = (OtherPawn->GetActorLocation() - TargetLoc).GetSafeNormal();
											if (FVector::DotProduct(TargetForward, TargetToAlly) > 0.80f)
											{
												bEngagingOther = true;
												break;
											}
										}
									}
								}
							}
						}
					}
				}
				else
				{
					// CLEAR true→false only when target is clearly facing us again (>= 0.65 ≈ 49°).
					if (DotToMe >= 0.65f)
					{
						bEngagingOther = false;
					}
				}
			}
			else
			{
				// No valid target — always clear the flag.
				bEngagingOther = false;
			}

			// Only touch the blackboard when the value actually changes.
			if (bEngagingOther != MyMemory->bCachedEngagingOther)
			{
				MyMemory->bCachedEngagingOther = bEngagingOther;
				BlackboardComp->SetValueAsBool(IsTargetEngagingOtherKey.SelectedKeyName, bEngagingOther);
			}
		}

		// ── Cover-time accumulator ────────────────────────────────────────────
		//
		// bCoverTimeInitialized prevents a spurious reset on the first tick after
		// a BT abort re-creates the node memory mid-fight (same guard pattern as
		// BTService_AIStateObserver::bInitialized).
		if (TargetTimeInCoverKey.IsSet() && MyMemory)
		{
			if (!MyMemory->bCoverTimeInitialized)
			{
				// First tick after (re)init: snapshot without resetting the accumulator.
				MyMemory->bCoverTimeInitialized = true;
			}
			else if (bTargetInCover)
			{
				MyMemory->TargetCoverTime += DeltaSeconds;
				BlackboardComp->SetValueAsFloat(TargetTimeInCoverKey.SelectedKeyName, MyMemory->TargetCoverTime);
			}
			else
			{
				if (MyMemory->TargetCoverTime != 0.f)
				{
					MyMemory->TargetCoverTime = 0.f;
					BlackboardComp->SetValueAsFloat(TargetTimeInCoverKey.SelectedKeyName, 0.f);
				}
			}
		}
	}
}
