// Copyright MyShooterScenarios. All Rights Reserved.

#include "AI/BTService_CoverModeSync.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/LyraCharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTService_CoverModeSync)

UBTService_CoverModeSync::UBTService_CoverModeSync(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = "Cover Mode Sync (MYST)";
	bNotifyTick = true;

	// 0.1 s is fast enough to catch cover entry/exit within 2 frames at 60 Hz
	// while avoiding per-frame overhead.
	Interval = 0.10f;
	RandomDeviation = 0.05f;

	IsInCoverModeKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_CoverModeSync, IsInCoverModeKey));
}

void UBTService_CoverModeSync::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIController || !BB)
	{
		return;
	}

	if (IsInCoverModeKey.SelectedKeyName == NAME_None)
	{
		return;
	}

	APawn* AIPawn = AIController->GetPawn();
	if (!AIPawn)
	{
		// Pawn not spawned yet — ensure key stays false
		BB->SetValueAsBool(IsInCoverModeKey.SelectedKeyName, false);
		return;
	}

	ULyraCharacterMovementComponent* CMC = AIPawn->FindComponentByClass<ULyraCharacterMovementComponent>();
	const bool bInCover = CMC && CMC->IsInCoverMode();

	// Only write when the value actually changes to avoid triggering unnecessary
	// Observer-Abort re-evaluations on decorators that watch this key.
	const bool bCurrentBBValue = BB->GetValueAsBool(IsInCoverModeKey.SelectedKeyName);
	if (bInCover != bCurrentBBValue)
	{
		BB->SetValueAsBool(IsInCoverModeKey.SelectedKeyName, bInCover);

		UE_LOG(LogTemp, Log, TEXT("BTService_CoverModeSync [%s]: IsInCoverMode BB key → %s"),
			*GetNameSafe(AIPawn), bInCover ? TEXT("TRUE") : TEXT("FALSE"));
	}
}

FString UBTService_CoverModeSync::GetStaticDescription() const
{
	return FString::Printf(TEXT("Syncs CMC::IsInCoverMode() → BB key '%s' (%.2f s interval)"),
		*IsInCoverModeKey.SelectedKeyName.ToString(), Interval);
}

