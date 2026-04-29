// Copyright MyShooterScenarios. All Rights Reserved.

#include "AI/BTTask_FindPeekLocation.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
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
	TargetEnemyKey.AddObjectFilter(
		this, GET_MEMBER_NAME_CHECKED(UBTTask_FindPeekLocation, TargetEnemyKey),
		AActor::StaticClass());
	PeekLocationKey.AddVectorFilter(
		this, GET_MEMBER_NAME_CHECKED(UBTTask_FindPeekLocation, PeekLocationKey));
}

void UBTTask_FindPeekLocation::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		CoverLocationKey.ResolveSelectedKey(*BBAsset);
		TargetEnemyKey.ResolveSelectedKey(*BBAsset);
		PeekLocationKey.ResolveSelectedKey(*BBAsset);
	}
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

	if (!CoverLocationKey.IsSet() || !TargetEnemyKey.IsSet() || !PeekLocationKey.IsSet())
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_FindPeekLocation: one or more BB keys are not set on node '%s'"), *NodeName);
		return EBTNodeResult::Failed;
	}

	const FVector CoverLocation = BB->GetValueAsVector(CoverLocationKey.SelectedKeyName);

	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(TargetEnemyKey.SelectedKeyName));
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

	// Strafe axis = right side when facing the target.
	// CrossProduct(Up, Forward) → right in UE's left-handed coordinate system.
	const FVector StrafeAxis =
		FVector::CrossProduct(FVector::UpVector, CoverToTarget2D).GetSafeNormal();

	// ── Target eye position ────────────────────────────────────────────────
	//
	// GetActorLocation() for ACharacter returns the capsule *centre* (half-height
	// above the ground), so naively adding EyeHeightOffset overshoot the real
	// eye by a full half-height.  GetActorEyesViewPoint() returns the correct
	// world-space eye location regardless of character type.
	FVector TargetEyePos;
	{
		FRotator TargetEyeRot;
		TargetActor->GetActorEyesViewPoint(TargetEyePos, TargetEyeRot);
	}

	// ── AI eye height delta ────────────────────────────────────────────────
	//
	// Derive the eye-height delta from the *actual* pawn so candidate eye
	// positions scale correctly regardless of character configuration.
	// EyeHeightOffset is kept as a designer fallback when the pawn is not an
	// ACharacter (e.g. flying/vehicle pawns whose eye view might not be set up).
	float EyeHeightAboveBase = EyeHeightOffset;
	{
		FVector PawnEyeWorldPos;
		FRotator PawnEyeWorldRot;
		AIPawn->GetActorEyesViewPoint(PawnEyeWorldPos, PawnEyeWorldRot);
		const float Delta = PawnEyeWorldPos.Z - AIPawn->GetActorLocation().Z;
		// Sanity-check: only use the live delta if it looks plausible (> 10 cm)
		if (Delta > 10.f)
		{
			EyeHeightAboveBase = Delta;
		}
	}

	// Ignore both the AI pawn and the target actor in every LOS trace so that
	// neither actor's own collision produces a false block.
	FCollisionQueryParams TraceParams(
		SCENE_QUERY_STAT(BTTask_FindPeekLocation), /*bTraceComplex=*/false);
	TraceParams.AddIgnoredActor(AIPawn);
	TraceParams.AddIgnoredActor(TargetActor);

	// ── Cover pre-check ────────────────────────────────────────────────────
	//
	// If CoverLocation already has clear LOS to the target on TraceChannel, the
	// EQS cover query and this task are likely using mismatched trace channels
	// (e.g. EQS used "Lyra_TraceChannel_Cover" while this task defaults to
	// ECC_Visibility).  In that case every lateral candidate will also appear
	// "clear", so we'd just pick the first arbitrary step.  Instead, treat the
	// cover location itself as the peek spot and warn the designer.
	{
		const FVector CoverEye = CoverLocation + FVector(0.f, 0.f, EyeHeightAboveBase);
		FHitResult CoverHit;
		const bool bCoverBlocked = World->LineTraceSingleByChannel(
			CoverHit, CoverEye, TargetEyePos, TraceChannel, TraceParams);
		const bool bCoverHasClearLOS =
			!bCoverBlocked || (CoverHit.GetActor() == TargetActor);

		if (bCoverHasClearLOS)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("BTTask_FindPeekLocation ('%s'): CoverLocation already has clear LOS "
				     "to target on channel %d.  The EQS cover query and this task may be "
				     "using different trace channels — verify TraceChannel matches the EQS "
				     "cover-visibility test.  Using CoverLocation as the peek spot."),
				*NodeName, (int32)TraceChannel.GetValue());

			BB->SetValueAsVector(PeekLocationKey.SelectedKeyName, CoverLocation);
			return EBTNodeResult::Succeeded;
		}
	}

	// Optional: NavSystem for projection
	UNavigationSystemV1* NavSys = bRequireNavMesh
		? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World)
		: nullptr;

	const FVector NavExtent(50.f, 50.f, NavProjectionExtentZ);

	// ── Sample candidates: +1, -1, +2, -2, … (closest first) ─────────────
	//
	// We record the first (closest) valid find and its distance bounds so the
	// binary search refinement below can push it back toward CoverLocation.

	bool      bFoundCandidate = false;
	FVector   BestNavCandidate = FVector::ZeroVector;
	FVector   PeekDirection    = FVector::ZeroVector; // unit strafe direction for refinement
	float     LoDistance       = 0.f;                 // last BLOCKED distance from CoverLocation
	float     HiDistance       = 0.f;                 // first CLEAR  distance from CoverLocation

	for (int32 Step = 1; Step <= MaxSteps && !bFoundCandidate; ++Step)
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
					continue; // Not on the nav mesh — skip
				}
				NavCandidate = NavResult.Location;
			}

			// Eye position for this candidate: nav-projected floor Z + live eye delta.
			const FVector CandidateEye = NavCandidate + FVector(0.f, 0.f, EyeHeightAboveBase);

			// ── Bidirectional LOS check ───────────────────────────────────
			//
			// A forward-only trace (candidate → target) silently passes when
			// CandidateEye is INSIDE blocking geometry: UE line traces that
			// START inside a convex mesh report no hit.  The reverse trace
			// (target → candidate) begins in open space and will correctly
			// report a block when the candidate is embedded in geometry.
			// Both directions must be clear.

			FHitResult ForwardHit;
			if (World->LineTraceSingleByChannel(
				ForwardHit, CandidateEye, TargetEyePos, TraceChannel, TraceParams))
			{
				continue; // Something between candidate and target.
			}

			FHitResult ReverseHit;
			if (World->LineTraceSingleByChannel(
				ReverseHit, TargetEyePos, CandidateEye, TraceChannel, TraceParams))
			{
				continue; // Candidate is embedded in or behind geometry.
			}

			// Valid — record and break out of both loops.
			BestNavCandidate = NavCandidate;
			PeekDirection    = StrafeAxis * (float)Sign;
			HiDistance       = StepSize * Step;
			LoDistance       = StepSize * (Step - 1); // 0 when Step == 1 (cover itself was blocked)
			bFoundCandidate  = true;
			break;
		}
	}

	if (!bFoundCandidate)
	{
		// No suitable peek position found — caller should keep the AI in cover.
		return EBTNodeResult::Failed;
	}

	// ── Binary search refinement ───────────────────────────────────────────
	//
	// We know:
	//   LoDistance  → last distance that was BLOCKED (or 0 = the cover itself)
	//   HiDistance  → first distance that was CLEAR  (BestNavCandidate)
	//
	// Each pass halves the gap; after BinarySearchIterations passes the result
	// is within StepSize / 2^N of the true cover edge.
	for (int32 i = 0; i < BinarySearchIterations; ++i)
	{
		const float   MidDist      = (LoDistance + HiDistance) * 0.5f;
		const FVector MidCandidate = CoverLocation + PeekDirection * MidDist;

		FVector MidNavCandidate = MidCandidate;
		if (NavSys)
		{
			FNavLocation MidNavResult;
			if (!NavSys->ProjectPointToNavigation(MidCandidate, MidNavResult, NavExtent))
			{
				// Can't reach this midpoint via nav; treat as blocked.
				LoDistance = MidDist;
				continue;
			}
			MidNavCandidate = MidNavResult.Location;
		}

		const FVector MidEye = MidNavCandidate + FVector(0.f, 0.f, EyeHeightAboveBase);

		FHitResult FwdHit, RevHit;
		const bool bMidBlocked =
			World->LineTraceSingleByChannel(FwdHit, MidEye,       TargetEyePos, TraceChannel, TraceParams) ||
			World->LineTraceSingleByChannel(RevHit, TargetEyePos, MidEye,       TraceChannel, TraceParams);

		if (bMidBlocked)
		{
			LoDistance = MidDist; // Too close to cover — search farther half.
		}
		else
		{
			HiDistance       = MidDist;         // Still clear — search closer half.
			BestNavCandidate = MidNavCandidate; // Closer valid candidate.
		}
	}

	BB->SetValueAsVector(PeekLocationKey.SelectedKeyName, BestNavCandidate);
	return EBTNodeResult::Succeeded;
}

FString UBTTask_FindPeekLocation::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("Cover:%s  Target:%s  →  Peek:%s\n"
			 "Steps:%d × %.0fcm  Refine:%d  EyeH:%.0fcm  NavMesh:%s"),
		*CoverLocationKey.SelectedKeyName.ToString(),
		*TargetEnemyKey.SelectedKeyName.ToString(),
		*PeekLocationKey.SelectedKeyName.ToString(),
		MaxSteps, StepSize, BinarySearchIterations, EyeHeightOffset,
		bRequireNavMesh ? TEXT("on") : TEXT("off"));
}


