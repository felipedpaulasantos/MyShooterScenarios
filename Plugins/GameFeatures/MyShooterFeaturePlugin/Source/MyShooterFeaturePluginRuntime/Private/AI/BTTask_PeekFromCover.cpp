// Copyright MyShooterScenarios. All Rights Reserved.

#include "AI/BTTask_PeekFromCover.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Character/LyraCharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTTask_PeekFromCover)

// Tag granted by the cover component when the AI reaches the cover edge.
static const FGameplayTag CanLeanTag = FGameplayTag::RequestGameplayTag(FName("Status.Cover.CanLean"));

UBTTask_PeekFromCover::UBTTask_PeekFromCover(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = "Peek From Cover (MYST)";

	bNotifyTick = true;
	bNotifyTaskFinished = true;

	TargetEnemyKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_PeekFromCover, TargetEnemyKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_PeekFromCover::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	// Must be in cover mode — the tangent is only valid there.
	ULyraCharacterMovementComponent* CMC = AIPawn->FindComponentByClass<ULyraCharacterMovementComponent>();
	if (!CMC || !CMC->IsInCoverMode())
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_PeekFromCover [%s]: Not in cover mode — failing."), *GetNameSafe(AIPawn));
		return EBTNodeResult::Failed;
	}

	AActor* TargetEnemy = Cast<AActor>(BB->GetValueAsObject(TargetEnemyKey.SelectedKeyName));
	if (!TargetEnemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_PeekFromCover [%s]: No TargetEnemy in blackboard — failing."), *GetNameSafe(AIPawn));
		return EBTNodeResult::Failed;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return EBTNodeResult::Failed;
	}

	// ── Cache strafe direction ONCE ──────────────────────────────────────────
	// Project the 2-D direction toward the enemy onto the cover surface tangent.
	// The sign determines which side to strafe toward; caching it prevents
	// sign oscillation as the dot product changes during movement.
	FVector ToEnemy2D = TargetEnemy->GetActorLocation() - AIPawn->GetActorLocation();
	ToEnemy2D.Z = 0.f;

	const float DotToTangent = FVector::DotProduct(ToEnemy2D.GetSafeNormal(), CMC->CoverSurfaceTangent);
	const float StrafeSign = FMath::Sign(DotToTangent);

	// Use placement new so the struct default-initialises correctly on raw BT memory.
	FBTPeekFromCoverMemory* Memory = new(NodeMemory) FBTPeekFromCoverMemory();
	Memory->State = EPeekState::Strafing;
	Memory->StartTime = World->GetTimeSeconds();
	Memory->CachedStrafeDir = CMC->CoverSurfaceTangent * (StrafeSign != 0.f ? StrafeSign : 1.f);

	UE_LOG(LogTemp, Log, TEXT("BTTask_PeekFromCover [%s]: Starting strafe. CachedStrafeDir=%s  Tangent=%s  DotToTangent=%.2f"),
		*GetNameSafe(AIPawn),
		*Memory->CachedStrafeDir.ToString(),
		*CMC->CoverSurfaceTangent.ToString(),
		DotToTangent);

	return EBTNodeResult::InProgress;
}

void UBTTask_PeekFromCover::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
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

	// Verify we're still in cover — if we fell out, abort cleanly.
	ULyraCharacterMovementComponent* CMC = AIPawn->FindComponentByClass<ULyraCharacterMovementComponent>();
	if (!CMC || !CMC->IsInCoverMode())
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_PeekFromCover [%s]: Lost cover mode mid-strafe — failing."), *GetNameSafe(AIPawn));
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FBTPeekFromCoverMemory* Memory = CastInstanceNodeMemory<FBTPeekFromCoverMemory>(NodeMemory);

	UWorld* World = GetWorld();
	if (!World)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// ── Check Status.Cover.CanLean tag ─────────────────────────────────────────────
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AIPawn);
	if (ASC && ASC->HasMatchingGameplayTag(CanLeanTag))
	{
		UE_LOG(LogTemp, Log, TEXT("BTTask_PeekFromCover [%s]: Status.Cover.CanLean received — peek position reached. Succeeding."),
			*GetNameSafe(AIPawn));
		Memory->State = EPeekState::Complete;
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// ── Timeout guard ────────────────────────────────────────────────────────
	const float Elapsed = World->GetTimeSeconds() - Memory->StartTime;
	if (Elapsed >= StrafeTimeout)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_PeekFromCover [%s]: Strafe TIMEOUT (%.1f s) — Status.Cover.CanLean never arrived. Failing."),
			*GetNameSafe(AIPawn), Elapsed);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// ── Drive lateral movement ───────────────────────────────────────────────
	AIPawn->AddMovementInput(Memory->CachedStrafeDir, StrafeSpeedScale);

#if ENABLE_DRAW_DEBUG
	DrawDebugString(World, AIPawn->GetActorLocation() + FVector(0.f, 0.f, 120.f),
		FString::Printf(TEXT("Peeking… %.1f / %.1f s"), Elapsed, StrafeTimeout),
		nullptr, FColor::Cyan, 0.f, false, 1.f);
#endif
}

EBTNodeResult::Type UBTTask_PeekFromCover::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// Movement input naturally stops when we stop calling AddMovementInput.
	return EBTNodeResult::Aborted;
}

FString UBTTask_PeekFromCover::GetStaticDescription() const
{
	return FString::Printf(TEXT("Strafe along cover toward enemy\nStop on: Status.Cover.CanLean  |  Timeout: %.1f s"), StrafeTimeout);
}

uint16 UBTTask_PeekFromCover::GetInstanceMemorySize() const
{
	return sizeof(FBTPeekFromCoverMemory);
}
