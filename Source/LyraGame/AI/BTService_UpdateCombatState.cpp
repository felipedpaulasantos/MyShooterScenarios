// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/BTService_UpdateCombatState.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/LyraHealthComponent.h"
#include "Teams/LyraTeamSubsystem.h"
#include "GameFramework/Character.h"
#include "EngineUtils.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

UBTService_UpdateCombatState::UBTService_UpdateCombatState()
{
	NodeName = "Update Combat State";
	bNotifyTick = true;
	IsolationRadius = 1500.0f;
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

	// Update Health Percentage
	if (HealthPercentageKey.IsSet())
	{
		if (ULyraHealthComponent* HealthComp = ULyraHealthComponent::FindHealthComponent(ControlledPawn))
		{
			float HealthPct = HealthComp->GetHealthNormalized();
			BlackboardComp->SetValueAsFloat(HealthPercentageKey.SelectedKeyName, HealthPct);
		}
	}

	// Update Is Isolated
	if (IsIsolatedKey.IsSet() && ControlledPawn->GetWorld())
	{
		bool bIsIsolated = true;
		if (ULyraTeamSubsystem* TeamSubsystem = ControlledPawn->GetWorld()->GetSubsystem<ULyraTeamSubsystem>())
		{
			bool bIsPartOfTeam = false;
			int32 MyTeamId = INDEX_NONE;
			TeamSubsystem->FindTeamFromActor(ControlledPawn, bIsPartOfTeam, MyTeamId);

			if (bIsPartOfTeam)
			{
				// Simple distance check to fast-track isolation. Could be improved with queries.
				const FVector MyLocation = ControlledPawn->GetActorLocation();
				const float RadiusSq = IsolationRadius * IsolationRadius;

				for (TActorIterator<APawn> It(ControlledPawn->GetWorld()); It; ++It)
				{
					APawn* OtherPawn = *It;
					if (OtherPawn != ControlledPawn && !OtherPawn->IsPlayerControlled()) // Assuming we care about other non-player allies
					{
						bool bOtherInTeam = false;
						int32 OtherTeamId = INDEX_NONE;
						TeamSubsystem->FindTeamFromActor(OtherPawn, bOtherInTeam, OtherTeamId);

						if (bOtherInTeam && OtherTeamId == MyTeamId)
						{
							if (FVector::DistSquared(MyLocation, OtherPawn->GetActorLocation()) <= RadiusSq)
							{
								bIsIsolated = false;
								break;
							}
						}
					}
				}
			}
		}
		BlackboardComp->SetValueAsBool(IsIsolatedKey.SelectedKeyName, bIsIsolated);
	}

	// Target in Cover tracking
	if (TargetKey.IsSet() && HasTargetInCoverKey.IsSet())
	{
		AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));
		bool bTargetInCover = false;
		
		if (TargetActor)
		{
			if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor))
			{
				// In Lyra, entering cover usually grants a GameplayTag (e.g., "State.Cover")
				// We abstract it via a check here. If no tag is used, custom project systems should substitute it.
				static FGameplayTag CoverTag = FGameplayTag::RequestGameplayTag(FName("State.Cover"), false);
				if (CoverTag.IsValid() && TargetASC->HasMatchingGameplayTag(CoverTag))
				{
					bTargetInCover = true;
				}
			}
			
			// Is target engaging someone else?
			if (IsTargetEngagingOtherKey.IsSet())
			{
				bool bEngagingOther = false;
				FVector TargetForward = TargetActor->GetActorForwardVector();
				FVector TargetLoc = TargetActor->GetActorLocation();
				FVector MyLoc = ControlledPawn->GetActorLocation();
				
				// Very basic approximation: If the target is not facing me, but is facing towards another of my allies
				FVector TargetToMe = (MyLoc - TargetLoc).GetSafeNormal();
				if (FVector::DotProduct(TargetForward, TargetToMe) < 0.5f)
				{
					// Target isn't looking closely at me. Are they looking at an ally?
					if (ULyraTeamSubsystem* TeamSubsystem = ControlledPawn->GetWorld()->GetSubsystem<ULyraTeamSubsystem>())
					{
						int32 MyTeamId = TeamSubsystem->FindTeamFromObject(ControlledPawn);
						if (MyTeamId != INDEX_NONE)
						{
							for (TActorIterator<APawn> It(ControlledPawn->GetWorld()); It; ++It)
							{
								APawn* OtherPawn = *It;
								if (OtherPawn && OtherPawn != ControlledPawn && OtherPawn != TargetActor)
								{
									if (TeamSubsystem->FindTeamFromObject(OtherPawn) == MyTeamId)
									{
										FVector TargetToAlly = (OtherPawn->GetActorLocation() - TargetLoc).GetSafeNormal();
										if (FVector::DotProduct(TargetForward, TargetToAlly) > 0.8f) // Looking at ally
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
				BlackboardComp->SetValueAsBool(IsTargetEngagingOtherKey.SelectedKeyName, bEngagingOther);
			}
		}
		
		BlackboardComp->SetValueAsBool(HasTargetInCoverKey.SelectedKeyName, bTargetInCover);

		// Record time in cover
		if (bTargetInCover)
		{
			if (MyMemory)
			{
				MyMemory->TargetCoverTime += DeltaSeconds;
				BlackboardComp->SetValueAsFloat(TargetTimeInCoverKey.SelectedKeyName, MyMemory->TargetCoverTime);
			}
		}
		else
		{
			if (MyMemory)
			{
				MyMemory->TargetCoverTime = 0.f;
				BlackboardComp->SetValueAsFloat(TargetTimeInCoverKey.SelectedKeyName, 0.f);
			}
		}
	}
}
