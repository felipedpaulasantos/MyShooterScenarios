// Copyright MyShooterScenarios. All Rights Reserved.

#include "AI/BTTask_FindPeekLocation.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Engine/World.h"
#include "NavigationSystem.h"

UBTTask_FindPeekLocation::UBTTask_FindPeekLocation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Find Peek Location");

	CoverLocationKey.AddVectorFilter(
		this, GET_MEMBER_NAME_CHECKED(UBTTask_FindPeekLocation, CoverLocationKey));
	TargetActorKey.AddObjectFilter(
		this, GET_MEMBER_NAME_CHECKED(UBTTask_FindPeekLocation, TargetActorKey),
		AActor::StaticClass());
	PeekLocationKey.AddVectorFilter(
		this, GET_MEMBER_NAME_CHECKED(UBTTask_FindPeekLocation, PeekLocationKey));
}

EBTNodeResult::Type UBTTask_FindPeekLocation::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC)
	{
		return EBTNodeResult::Failed;
	}

	APawn* AIPawn = AIC->GetPawn();
	if (!AIPawn)
	{
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
	{
		return EBTNodeResult::Failed;
	}

	// ── Gather inputs ──────────────────────────────────────────────────────

	const FVector CoverLocation = BB->GetValueAsVector(CoverLocationKey.SelectedKeyName);

	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!IsValid(TargetActor))
	{
		return EBTNodeResult::Failed;
	}

	UWorld* World = AIPawn->GetWorld();
	if (!World)
	{
		return EBTNodeResult::Failed;
	}

	// ── Build the strafe axis ──────────────────────────────────────────────
	//
	// We project to 2D so the strafe axis always lies on the horizontal plane,
	// preventing the AI from trying to step "into the ceiling".

	const FVector TargetLocation = TargetActor->GetActorLocation();
	const FVector CoverToTarget2D = (TargetLocation - CoverLocation).GetSafeNormal2D();

	if (CoverToTarget2D.IsNearlyZero())
	{
		// Directly above / below cover – use the AI's forward as fallback
		// (shouldn't happen in normal scenarios)
		return EBTNodeResult::Failed;
	}

	// Strafe axis = right side when facing the target
	const FVector StrafeAxis =
		FVector::CrossProduct(CoverToTarget2D, FVector::UpVector).GetSafeNormal();

	const FVector EyeOffset(0.f, 0.f, EyeHeightOffset);
	const FVector TargetEyePos = TargetLocation + EyeOffset;

	// Ignore the AI pawn in LOS traces
	FCollisionQueryParams TraceParams(
		SCENE_QUERY_STAT(BTTask_FindPeekLocation), /*bTraceComplex=*/false, AIPawn);

	// Optional: NavSystem for projection
	UNavigationSystemV1* NavSys = bRequireNavMesh
		? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World)
		: nullptr;

	const FVector NavExtent(50.f, 50.f, NavProjectionExtentZ);

	// ── Sample candidates: +1, -1, +2, -2, … (closest first) ─────────────

	for (int32 Step = 1; Step <= MaxSteps; ++Step)
	{
		for (const int32 Sign : {1, -1})
		{
			const FVector Candidate = CoverLocation + StrafeAxis * (StepSize * Step * Sign);

			// ── NavMesh projection ─────────────────────────────────────────
			FVector NavCandidate = Candidate;
			if (NavSys)
			{
				FNavLocation NavResult;
				if (!NavSys->ProjectPointToNavigation(Candidate, NavResult, NavExtent))
				{
					// Not on the nav mesh — skip
					continue;
				}
				NavCandidate = NavResult.Location;
			}

			// ── LOS trace from candidate eye position to target eye ────────
			const FVector CandidateEye = NavCandidate + EyeOffset;
			FHitResult Hit;
			const bool bBlocked = World->LineTraceSingleByChannel(
				Hit, CandidateEye, TargetEyePos, TraceChannel, TraceParams);

			// Clear if nothing was hit, or if the first thing hit IS the target
			const bool bClearLOS = !bBlocked || (Hit.GetActor() == TargetActor);

			if (bClearLOS)
			{
				BB->SetValueAsVector(PeekLocationKey.SelectedKeyName, NavCandidate);
				return EBTNodeResult::Succeeded;
			}
		}
	}

	// No suitable peek position found — caller should keep the AI in cover
	return EBTNodeResult::Failed;
}

FString UBTTask_FindPeekLocation::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("Cover:%s  Target:%s  →  Peek:%s\n"
			 "Steps:%d × %.0fcm  EyeH:%.0fcm  NavMesh:%s"),
		*CoverLocationKey.SelectedKeyName.ToString(),
		*TargetActorKey.SelectedKeyName.ToString(),
		*PeekLocationKey.SelectedKeyName.ToString(),
		MaxSteps, StepSize, EyeHeightOffset,
		bRequireNavMesh ? TEXT("on") : TEXT("off"));
}


