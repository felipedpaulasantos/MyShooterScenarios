// Copyright MyShooterScenarios. All Rights Reserved.

#include "AI/BTTask_FindPeekLocation.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "NavigationSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTTask_FindPeekLocation)

UBTTask_FindPeekLocation::UBTTask_FindPeekLocation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = "Find Peek Location (MYST)";
	
	// This task completes in one frame (synchronous)
	bNotifyTick = false;
	
	CoverLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindPeekLocation, CoverLocationKey));
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindPeekLocation, TargetActorKey), AActor::StaticClass());
	PeekLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindPeekLocation, PeekLocationKey));
}

EBTNodeResult::Type UBTTask_FindPeekLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIController || !BB)
	{
		return EBTNodeResult::Failed;
	}
	
	APawn* AIPawn = AIController->GetPawn();
	if (!AIPawn)
	{
		return EBTNodeResult::Failed;
	}
	
	const FVector CoverLocation = BB->GetValueAsVector(CoverLocationKey.SelectedKeyName);
	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	
	if (CoverLocation.IsNearlyZero() || !TargetActor)
	{
		return EBTNodeResult::Failed;
	}
	
	// Compute the strafe axis perpendicular to the cover→target direction
	const FVector ToTarget = (TargetActor->GetActorLocation() - CoverLocation).GetSafeNormal2D();
	const FVector StrafeAxis = FVector::CrossProduct(FVector::UpVector, ToTarget).GetSafeNormal();
	
	UWorld* World = GetWorld();
	if (!World)
	{
		return EBTNodeResult::Failed;
	}
	
	// Probe left and right from the cover location
	const int32 Directions[2] = { -1, 1 };
	
	for (int32 Step = 1; Step <= MaxSteps; ++Step)
	{
		for (int32 Dir : Directions)
		{
			FVector Candidate = CoverLocation + StrafeAxis * (StepSize * Step * Dir);
			
			// Optional: project onto NavMesh
			if (bRequireNavMesh)
			{
				UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
				if (NavSys)
				{
					FNavLocation NavResult;
					const FVector Extent(0.f, 0.f, NavProjectionExtentZ);
					if (!NavSys->ProjectPointToNavigation(Candidate, NavResult, Extent))
					{
						// Not on NavMesh, skip this candidate
						continue;
					}
					Candidate = NavResult.Location;
				}
			}
			
			// Line trace from candidate (at eye height) to target
			const FVector TraceStart = Candidate + FVector(0.f, 0.f, EyeHeightOffset);
			const FVector TraceEnd = TargetActor->GetActorLocation();
			
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BTTask_FindPeekLocation), false, AIPawn);
			QueryParams.AddIgnoredActor(TargetActor);
			
			FHitResult Hit;
			const bool bBlocked = World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, TraceChannel, QueryParams);
			
			if (!bBlocked)
			{
				// Clear LOS found — write peek location and succeed
				BB->SetValueAsVector(PeekLocationKey.SelectedKeyName, Candidate);
				return EBTNodeResult::Succeeded;
			}
		}
	}
	
	// No peek location found
	return EBTNodeResult::Failed;
}

FString UBTTask_FindPeekLocation::GetStaticDescription() const
{
	return FString::Printf(TEXT("Find peek location from cover\nStepSize: %.0f cm, MaxSteps: %d"),
		StepSize, MaxSteps);
}

