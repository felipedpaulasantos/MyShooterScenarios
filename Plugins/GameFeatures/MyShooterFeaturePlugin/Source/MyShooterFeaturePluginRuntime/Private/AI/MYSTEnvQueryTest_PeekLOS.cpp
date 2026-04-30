// Copyright MyShooterScenarios. All Rights Reserved.

#include "AI/MYSTEnvQueryTest_PeekLOS.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"

#define LOCTEXT_NAMESPACE "MYSTEnvQueryTest_PeekLOS"

UMYSTEnvQueryTest_PeekLOS::UMYSTEnvQueryTest_PeekLOS(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ValidItemType  = UEnvQueryItemType_VectorBase::StaticClass();
	Cost           = EEnvTestCost::High; // two line traces per candidate
	TestPurpose    = EEnvTestPurpose::Filter;
	SetWorkOnFloatValues(false);
}

void UMYSTEnvQueryTest_PeekLOS::RunTest(FEnvQueryInstance& QueryInstance) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	if (!QueryOwner)
	{
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(QueryOwner, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return;
	}

	// ── Resolve AI pawn & controller ────────────────────────────────────────

	APawn* AIPawn = Cast<APawn>(QueryOwner);
	AAIController* AIC = AIPawn
		? Cast<AAIController>(AIPawn->GetController())
		: Cast<AAIController>(QueryOwner);

	if (!AIC)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UMYSTEnvQueryTest_PeekLOS: could not resolve AIController from query owner."));
		return;
	}

	// ── Target actor from Blackboard ─────────────────────────────────────────

	UBlackboardComponent* BB = AIC->GetBlackboardComponent();
	if (!BB)
	{
		return;
	}

	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(TargetActorKeyName));
	if (!IsValid(TargetActor))
	{
		// Target gone — fail every item so the BT task yields Failed
		for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
		{
			It.SetScore(TestPurpose, FilterType, false, true);
		}
		return;
	}

	// ── Trace height ─────────────────────────────────────────────────────────
	//
	// We trace from BARREL height, NOT eye height.
	// The eye (~160 cm) clears a cover corner cleanly, but the gun barrel
	// (~120 cm) still clips that same corner, making the AI shoot its own cover.
	// Tracing from barrel height means the EQS only passes positions where the
	// weapon itself has a clear shot.
	//
	// If TraceHeightCm == 0, derive the height live from GetActorEyesViewPoint()
	// as a fallback for non-standard characters.
	float TraceHeight = TraceHeightCm;
	if (TraceHeight < 1.f && IsValid(AIPawn))
	{
		FVector PawnEyePos;
		FRotator Unused;
		AIPawn->GetActorEyesViewPoint(PawnEyePos, Unused);
		const float Delta = PawnEyePos.Z - (AIPawn->GetActorLocation().Z - AIPawn->GetSimpleCollisionHalfHeight());
		if (Delta > 10.f) TraceHeight = Delta;
	}

	// ── Target trace position ────────────────────────────────────────────────
	//
	// Use the same height above the target's base as we use for the AI so both
	// ends of every trace are at a consistent "barrel height" plane.
	// GetActorLocation() on a character is the capsule centre; subtracting the
	// capsule half-height gives the floor, then we add TraceHeight.
	FVector TargetEyePos;
	{
		const float TargetFloorZ =
			TargetActor->GetActorLocation().Z - TargetActor->GetSimpleCollisionHalfHeight();
		TargetEyePos = FVector(TargetActor->GetActorLocation().X,
		                       TargetActor->GetActorLocation().Y,
		                       TargetFloorZ + TraceHeight);
	}

	// ── Trace params — ignore both the AI and the target ─────────────────────

	FCollisionQueryParams TraceParams(
		SCENE_QUERY_STAT(MYSTEnvQueryTest_PeekLOS), /*bTraceComplex=*/false);
	if (IsValid(AIPawn))     TraceParams.AddIgnoredActor(AIPawn);
	TraceParams.AddIgnoredActor(TargetActor);

	// ── Evaluate each candidate ───────────────────────────────────────────────

	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const FVector CandidateLoc = GetItemLocation(QueryInstance, It.GetIndex());
		const FVector CandidateEye = CandidateLoc + FVector(0.f, 0.f, TraceHeight);

		// Forward: geometry between candidate and target?
		FHitResult ForwardHit;
		const bool bFwdBlocked = World->LineTraceSingleByChannel(
			ForwardHit, CandidateEye, TargetEyePos, TraceChannel, TraceParams);

		if (bFwdBlocked)
		{
			It.SetScore(TestPurpose, FilterType, false, true);
			continue;
		}

		// Reverse: candidate inside / behind geometry?
		// Starts in open air at the target, so it cannot be fooled by interior starts.
		FHitResult ReverseHit;
		const bool bRevBlocked = World->LineTraceSingleByChannel(
			ReverseHit, TargetEyePos, CandidateEye, TraceChannel, TraceParams);

		It.SetScore(TestPurpose, FilterType, !bRevBlocked, true);
	}
}

FText UMYSTEnvQueryTest_PeekLOS::GetDescriptionTitle() const
{
	return LOCTEXT("Title", "Peek LOS to Target");
}

FText UMYSTEnvQueryTest_PeekLOS::GetDescriptionDetails() const
{
	return FText::Format(
		LOCTEXT("Details",
			"Bidirectional LOS to BB key '{0}' on channel {1} at height {2} cm.\n"
			"Filters candidates blocked by geometry or embedded inside it."),
		FText::FromName(TargetActorKeyName),
		FText::AsNumber(static_cast<int32>(TraceChannel.GetValue())),
		FText::AsNumber(FMath::RoundToInt(TraceHeightCm))
	);
}

#undef LOCTEXT_NAMESPACE


