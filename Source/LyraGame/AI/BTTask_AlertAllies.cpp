// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/BTTask_AlertAllies.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Teams/LyraTeamSubsystem.h"
#include "EngineUtils.h"

UBTTask_AlertAllies::UBTTask_AlertAllies()
{
	NodeName = "Alert Nearby Allies";
	AlertRadius = 2500.0f;
}

EBTNodeResult::Type UBTTask_AlertAllies::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	
	if (!AIController || !BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	APawn* ControlledPawn = AIController->GetPawn();
	if (!ControlledPawn || !ControlledPawn->GetWorld())
	{
		return EBTNodeResult::Failed;
	}

	UObject* TargetObject = BlackboardComp->GetValueAsObject(TargetToAlertKey.SelectedKeyName);
	AActor* TargetActor = Cast<AActor>(TargetObject);

	if (!TargetActor)
	{
		return EBTNodeResult::Failed;
	}

	int32 AlertedCount = 0;

	if (ULyraTeamSubsystem* TeamSubsystem = ControlledPawn->GetWorld()->GetSubsystem<ULyraTeamSubsystem>())
	{
		bool bIsPartOfTeam = false;
		int32 MyTeamId = INDEX_NONE;
		TeamSubsystem->FindTeamFromActor(ControlledPawn, bIsPartOfTeam, MyTeamId);

		if (bIsPartOfTeam)
		{
			const FVector MyLocation = ControlledPawn->GetActorLocation();
			const float RadiusSq = AlertRadius * AlertRadius;

			for (TActorIterator<APawn> It(ControlledPawn->GetWorld()); It; ++It)
			{
				APawn* OtherPawn = *It;
				if (OtherPawn && OtherPawn != ControlledPawn && !OtherPawn->IsPlayerControlled())
				{
					bool bOtherInTeam = false;
					int32 OtherTeamId = INDEX_NONE;
					TeamSubsystem->FindTeamFromActor(OtherPawn, bOtherInTeam, OtherTeamId);

					if (bOtherInTeam && OtherTeamId == MyTeamId)
					{
						if (FVector::DistSquared(MyLocation, OtherPawn->GetActorLocation()) <= RadiusSq)
						{
							// Found an ally in range, explicitly give them our target.
							if (AAIController* AllyController = Cast<AAIController>(OtherPawn->GetController()))
							{
								if (UBlackboardComponent* AllyBB = AllyController->GetBlackboardComponent())
								{
									// By convention assuming they use the same BB Key name for Target.
									// Or we can register a perception stimulus, but putting it straight into 
									// the blackboard is faster for simple reinforcement behaviors.
									AllyBB->SetValueAsObject(TargetToAlertKey.SelectedKeyName, TargetActor);
									AlertedCount++;
								}
							}
						}
					}
				}
			}
		}
	}

	return (AlertedCount > 0) ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}

