// Copyright MyShooterScenarios. All Rights Reserved.

#include "AI/BTTask_EnterCover.h"

#include "AIController.h"
#include "AI/MYSTCoverClaimSubsystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Character/LyraCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTTask_EnterCover)

UBTTask_EnterCover::UBTTask_EnterCover(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = "Enter Cover (MYST)";
	
	bNotifyTick = true;
	bNotifyTaskFinished = true;
	
	CoverLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_EnterCover, CoverLocationKey));
}

EBTNodeResult::Type UBTTask_EnterCover::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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
	if (CoverLocation.IsNearlyZero())
	{
		return EBTNodeResult::Failed;
	}
	
	UWorld* World = GetWorld();
	if (!World)
	{
		return EBTNodeResult::Failed;
	}
	
	// 1. Claim the cover spot
	UMYSTCoverClaimSubsystem* ClaimSys = World->GetSubsystem<UMYSTCoverClaimSubsystem>();
	if (!ClaimSys)
	{
		return EBTNodeResult::Failed;
	}
	
	if (!ClaimSys->ClaimSpot(CoverLocation, AIPawn, ClaimRadius))
	{
		// Spot already occupied
		return EBTNodeResult::Failed;
	}
	
	// Store memory — use placement new so default member initializers are applied.
	// CastInstanceNodeMemory is a plain reinterpret_cast; without this the raw
	// BT node memory is uninitialized and bCoverModeEntered may read as true.
	FBTEnterCoverMemory* Memory = new(NodeMemory) FBTEnterCoverMemory();
	Memory->bSpotClaimed = true;
	Memory->ClaimedLocation = CoverLocation;
	
	UE_LOG(LogTemp, Log, TEXT("BTTask_EnterCover [%s]: Claimed spot at %s — starting move"),
		*GetNameSafe(AIPawn), *CoverLocation.ToString());

	// 2. Start moving to cover location
	UAIBlueprintHelperLibrary::SimpleMoveToLocation(AIController, CoverLocation);

	// Draw the target cover location
	DrawDebugSphere(World, CoverLocation, 30.f, 12, FColor::Cyan, false, 5.f, 0, 2.f);
	
	// Continue in TickTask
	return EBTNodeResult::InProgress;
}

void UBTTask_EnterCover::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIController || !BB)
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
	
	UWorld* World = GetWorld();
	if (!World)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	
	FBTEnterCoverMemory* Memory = CastInstanceNodeMemory<FBTEnterCoverMemory>(NodeMemory);
	
	// Phase 2: Already entered cover mode — complete immediately.
	// IsInCoverMode BB key is now managed by BTService_CoverModeSync.
	if (Memory->bCoverModeEntered)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}
	
	// Phase 1: Moving to cover and entering cover mode
	const FVector CoverLocation = BB->GetValueAsVector(CoverLocationKey.SelectedKeyName);
	const float DistToCover = FVector::Dist2D(AIPawn->GetActorLocation(), CoverLocation);

	// Draw current distance to cover every tick (visible for 0.15s so it's readable without flooding)
	DrawDebugString(World, AIPawn->GetActorLocation() + FVector(0, 0, 100),
		FString::Printf(TEXT("DistToCover: %.1f (need <= %.1f)"), DistToCover, AcceptanceRadius),
		nullptr, FColor::Yellow, 0.15f);

	if (DistToCover > AcceptanceRadius)
	{
		// Still moving
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("BTTask_EnterCover [%s]: Arrived at cover (dist=%.1f). Starting wall trace."),
		*GetNameSafe(AIPawn), DistToCover);

	// 3. Arrived — fire wall trace to get FHitResult.
	// Use mid-body height and a purely horizontal direction so the trace hits the wall
	// rather than the floor (EQS cover points are navmesh-level, i.e. at foot height).
	ACharacter* AIChar = Cast<ACharacter>(AIPawn);
	const float MidBodyZ = AIChar
		? AIPawn->GetActorLocation().Z + AIChar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
		: AIPawn->GetActorLocation().Z + 90.f;

	const FVector TraceStart = FVector(AIPawn->GetActorLocation().X, AIPawn->GetActorLocation().Y, MidBodyZ);

	// Flatten direction to XY plane so the ray travels horizontally toward the wall
	FVector Dir2D = CoverLocation - AIPawn->GetActorLocation();
	Dir2D.Z = 0.f;
	const FVector TraceDir = Dir2D.GetSafeNormal();

	// Extend from the pawn, through the cover point, and WallTraceDistance beyond — all at mid-body height
	const FVector TraceEnd = FVector(CoverLocation.X, CoverLocation.Y, MidBodyZ) + TraceDir * WallTraceDistance;

	UE_LOG(LogTemp, Log, TEXT("BTTask_EnterCover [%s]: Trace horizontal — Start=%s End=%s MidBodyZ=%.1f"),
		*GetNameSafe(AIPawn), *TraceStart.ToString(), *TraceEnd.ToString(), MidBodyZ);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BTTask_EnterCover_WallTrace), false, AIPawn);
	FHitResult WallHit;

	const bool bHitSomething = World->LineTraceSingleByChannel(WallHit, TraceStart, TraceEnd, TraceChannel, QueryParams);

	// Always draw the trace so we can see where it went and what it hit
	const FColor TraceColor = bHitSomething ? FColor::Green : FColor::Red;
	DrawDebugLine(World, TraceStart, TraceEnd, TraceColor, false, 5.f, 0, 2.f);

	if (bHitSomething)
	{
		DrawDebugSphere(World, WallHit.ImpactPoint, 15.f, 8, FColor::Orange, false, 5.f, 0, 2.f);
		DrawDebugString(World, WallHit.ImpactPoint + FVector(0, 0, 50),
			FString::Printf(TEXT("Hit: %s | Component: %s"), *GetNameSafe(WallHit.GetActor()), *GetNameSafe(WallHit.GetComponent())),
			nullptr, FColor::Orange, 5.f);

		UE_LOG(LogTemp, Log, TEXT("BTTask_EnterCover [%s]: Wall trace HIT — Actor: %s | Component: %s | Normal: %s"),
			*GetNameSafe(AIPawn),
			*GetNameSafe(WallHit.GetActor()),
			*GetNameSafe(WallHit.GetComponent()),
			*WallHit.ImpactNormal.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_EnterCover [%s]: Wall trace MISSED — TraceStart: %s | TraceEnd: %s | Channel: %d"),
			*GetNameSafe(AIPawn),
			*TraceStart.ToString(),
			*TraceEnd.ToString(),
			(int32)TraceChannel);
	}

	// Validate the hit: must be blocking AND tagged with the required cover tag
	bool bValidCoverHit = false;
	if (bHitSomething && WallHit.bBlockingHit)
	{
		// Check if the hit component or its owner actor has the required cover tag
		AActor* HitActor = WallHit.GetActor();
		UPrimitiveComponent* HitComponent = WallHit.GetComponent();

		if (HitComponent && HitComponent->ComponentHasTag(RequiredCoverTag))
		{
			bValidCoverHit = true;
			UE_LOG(LogTemp, Log, TEXT("BTTask_EnterCover [%s]: Cover tag '%s' found on COMPONENT '%s'"),
				*GetNameSafe(AIPawn), *RequiredCoverTag.ToString(), *GetNameSafe(HitComponent));
		}
		else if (HitActor && HitActor->ActorHasTag(RequiredCoverTag))
		{
			bValidCoverHit = true;
			UE_LOG(LogTemp, Log, TEXT("BTTask_EnterCover [%s]: Cover tag '%s' found on ACTOR '%s'"),
				*GetNameSafe(AIPawn), *RequiredCoverTag.ToString(), *GetNameSafe(HitActor));
		}
		else
		{
			// Log what tags the hit actor/component actually has to help diagnose missing tag
			UE_LOG(LogTemp, Warning, TEXT("BTTask_EnterCover [%s]: Hit actor/component does NOT have cover tag '%s'. Required tag missing!"),
				*GetNameSafe(AIPawn), *RequiredCoverTag.ToString());

			if (HitActor)
			{
				FString ActorTags;
				for (const FName& Tag : HitActor->Tags)
				{
					ActorTags += Tag.ToString() + TEXT(", ");
				}
				UE_LOG(LogTemp, Warning, TEXT("  Actor '%s' tags: [%s]"), *GetNameSafe(HitActor), *ActorTags);
			}
			if (HitComponent)
			{
				FString CompTags;
				for (const FName& Tag : HitComponent->ComponentTags)
				{
					CompTags += Tag.ToString() + TEXT(", ");
				}
				UE_LOG(LogTemp, Warning, TEXT("  Component '%s' tags: [%s]"), *GetNameSafe(HitComponent), *CompTags);
			}

			// Draw a red X at the impact point to indicate tag failure
			DrawDebugSphere(World, WallHit.ImpactPoint, 20.f, 8, FColor::Red, false, 5.f, 0, 3.f);
			DrawDebugString(World, WallHit.ImpactPoint + FVector(0, 0, 80),
				FString::Printf(TEXT("MISSING TAG '%s'"), *RequiredCoverTag.ToString()),
				nullptr, FColor::Red, 5.f);
		}
	}
	else if (bHitSomething && !WallHit.bBlockingHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_EnterCover [%s]: Trace hit something but it was NOT blocking. Check collision settings on cover mesh."),
			*GetNameSafe(AIPawn));
	}
	
	if (!bValidCoverHit)
	{
		// No valid cover wall found — bad cover location (probably hit ground or non-cover object)
		UE_LOG(LogTemp, Warning, TEXT("BTTask_EnterCover [%s]: No valid cover hit — aborting task. bHitSomething=%d"),
			*GetNameSafe(AIPawn), (int32)bHitSomething);

		if (Memory->bSpotClaimed)
		{
			UMYSTCoverClaimSubsystem* ClaimSys = World->GetSubsystem<UMYSTCoverClaimSubsystem>();
			if (ClaimSys)
			{
				ClaimSys->ReleaseSpot(AIPawn);
			}
			Memory->bSpotClaimed = false;
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	
	// 4. Enter cover mode
	ULyraCharacterMovementComponent* CMC = AIPawn->FindComponentByClass<ULyraCharacterMovementComponent>();
	if (!CMC)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_EnterCover [%s]: No ULyraCharacterMovementComponent found on pawn! Cannot enter cover."),
			*GetNameSafe(AIPawn));
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("BTTask_EnterCover [%s]: Calling EnterCoverMode. ImpactPoint=%s ImpactNormal=%s MaxSpeed=%.0f"),
		*GetNameSafe(AIPawn),
		*WallHit.ImpactPoint.ToString(),
		*WallHit.ImpactNormal.ToString(),
		CoverMaxSpeed);

	CMC->EnterCoverMode(WallHit, CoverMaxSpeed);

	// Draw the wall normal to confirm orientation
	DrawDebugLine(World, WallHit.ImpactPoint, WallHit.ImpactPoint + WallHit.ImpactNormal * 80.f, FColor::Blue, false, 5.f, 0, 3.f);
	DrawDebugString(World, WallHit.ImpactPoint + FVector(0, 0, 100),
		TEXT("EnterCoverMode CALLED"), nullptr, FColor::Green, 5.f);
	
	// 5. Mark that we've entered cover mode — task will complete on next tick.
	// BTService_CoverModeSync will pick up IsInCoverMode() and update the BB key.
	Memory->bCoverModeEntered = true;
}

EBTNodeResult::Type UBTTask_EnterCover::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Aborted;
	}
	
	APawn* AIPawn = AIController->GetPawn();
	if (AIPawn)
	{
		// Exit cover mode
		ULyraCharacterMovementComponent* CMC = AIPawn->FindComponentByClass<ULyraCharacterMovementComponent>();
		if (CMC && CMC->IsInCoverMode())
		{
			CMC->ExitCoverMode();
		}
		
		// Release claim
		FBTEnterCoverMemory* Memory = CastInstanceNodeMemory<FBTEnterCoverMemory>(NodeMemory);
		if (Memory->bSpotClaimed)
		{
			UWorld* World = GetWorld();
			if (World)
			{
				UMYSTCoverClaimSubsystem* ClaimSys = World->GetSubsystem<UMYSTCoverClaimSubsystem>();
				if (ClaimSys)
				{
					ClaimSys->ReleaseSpot(AIPawn);
				}
			}
			Memory->bSpotClaimed = false;
		}
	}
	
	return EBTNodeResult::Aborted;
}

FString UBTTask_EnterCover::GetStaticDescription() const
{
	return FString::Printf(TEXT("Enter cover at location\nSpeed: %.0f cm/s, Tag: %s"),
		CoverMaxSpeed, *RequiredCoverTag.ToString());
}

uint16 UBTTask_EnterCover::GetInstanceMemorySize() const
{
	return sizeof(FBTEnterCoverMemory);
}

