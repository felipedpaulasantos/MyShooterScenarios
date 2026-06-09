// Copyright MyShooterScenarios. All Rights Reserved.

#include "AI/BTTask_ExitCover.h"

#include "AIController.h"
#include "AI/MYSTCoverClaimSubsystem.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Character/LyraCharacterMovementComponent.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTTask_ExitCover)

UBTTask_ExitCover::UBTTask_ExitCover(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = "Exit Cover (MYST)";
	
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_ExitCover::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}
	
	APawn* AIPawn = AIController->GetPawn();
	if (!AIPawn)
	{
		return EBTNodeResult::Failed;
	}
	
	ULyraCharacterMovementComponent* CMC = AIPawn->FindComponentByClass<ULyraCharacterMovementComponent>();
	if (!CMC)
	{
		return EBTNodeResult::Failed;
	}
	
	// 1. Read cover surface normal before exiting
	const FVector CoverNormal = CMC->CoverSurfaceNormal;
	if (CoverNormal.IsNearlyZero())
	{
		// Not actually in cover
		return EBTNodeResult::Failed;
	}
	
	const FVector CurrentLocation = AIPawn->GetActorLocation();
	
	// 2. Exit cover mode (restores MOVE_Walking)
	CMC->ExitCoverMode();
	
	// 3. Release cover claim
	UWorld* World = GetWorld();
	if (World)
	{
		UMYSTCoverClaimSubsystem* ClaimSys = World->GetSubsystem<UMYSTCoverClaimSubsystem>();
		if (ClaimSys)
		{
			ClaimSys->ReleaseSpot(AIPawn);
		}
	}
	
	// 4. Compute retreat point: move away from wall perpendicular to surface
	const FVector RetreatLocation = CurrentLocation + CoverNormal * RetreatDistance;
	
	// Store in memory for TickTask
	FBTExitCoverMemory* Memory = CastInstanceNodeMemory<FBTExitCoverMemory>(NodeMemory);
	Memory->RetreatLocation = RetreatLocation;
	Memory->bRetreatStarted = false;
	
	// 6. Start retreat move
	UAIBlueprintHelperLibrary::SimpleMoveToLocation(AIController, RetreatLocation);
	Memory->bRetreatStarted = true;
	
	return EBTNodeResult::InProgress;
}

void UBTTask_ExitCover::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	
	APawn* AIPawn = AIController->GetPawn();
	if (!AIPawn)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	
	FBTExitCoverMemory* Memory = CastInstanceNodeMemory<FBTExitCoverMemory>(NodeMemory);
	if (!Memory->bRetreatStarted)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	
	const float DistToRetreat = FVector::Dist2D(AIPawn->GetActorLocation(), Memory->RetreatLocation);
	
	if (DistToRetreat <= AcceptanceRadius)
	{
		// Retreat complete
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

FString UBTTask_ExitCover::GetStaticDescription() const
{
	return FString::Printf(TEXT("Exit cover and retreat %.0f cm"), RetreatDistance);
}

uint16 UBTTask_ExitCover::GetInstanceMemorySize() const
{
	return sizeof(FBTExitCoverMemory);
}

