// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction/InteractableIconSelectorComponent.h"

#include "Interaction/InteractableIconInterface.h"

#include "DrawDebugHelpers.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"
#include "Kismet/KismetSystemLibrary.h"

// Small reflection helpers to call Blueprint Interface functions without generated headers.
namespace InteractableIconSelector::Private
{
	static UFunction* FindFunc(const UObject* Obj, const FName FuncName)
	{
		return Obj ? Obj->FindFunction(FuncName) : nullptr;
	}
}

UInteractableIconSelectorComponent::UInteractableIconSelectorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInteractableIconSelectorComponent::BeginPlay()
{
	Super::BeginPlay();

	StartTimerIfNeeded();
}

void UInteractableIconSelectorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopTimer();

	// Hide any currently selected icon on shutdown to avoid dangling UI.
	if (AActor* Best = CurrentBestActor.Get())
	{
		SetIconVisibilityOnActor(Best, false);
	}
	CurrentBestActor.Reset();
	CurrentBestScore = -1.0f;

	Super::EndPlay(EndPlayReason);
}

void UInteractableIconSelectorComponent::SetSelectorEnabled(bool bEnabled)
{
	bSelectorEnabled = bEnabled;
	if (bSelectorEnabled)
	{
		StartTimerIfNeeded();
		ForceScan();
	}
	else
	{
		StopTimer();
		// Hide all previously considered icons (at least the best).
		if (AActor* Best = CurrentBestActor.Get())
		{
			SetIconVisibilityOnActor(Best, false);
		}
		CurrentBestActor.Reset();
		CurrentBestScore = -1.0f;
	}
}

void UInteractableIconSelectorComponent::ForceScan()
{
	ScanAndApply();
}

void UInteractableIconSelectorComponent::StartTimerIfNeeded()
{
	if (!bSelectorEnabled)
	{
		return;
	}

	if (!IsLocallyControlledOwner())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (ScanTimerHandle.IsValid())
	{
		return;
	}

	World->GetTimerManager().SetTimer(ScanTimerHandle, this, &UInteractableIconSelectorComponent::ScanAndApply, ScanInterval, true);
}

void UInteractableIconSelectorComponent::StopTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ScanTimerHandle);
	}
	ScanTimerHandle.Invalidate();
}

bool UInteractableIconSelectorComponent::IsLocallyControlledOwner() const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	// In Standalone (single-player) we can always run locally.
	if (Owner->GetNetMode() == NM_Standalone)
	{
		return true;
	}

	const APawn* PawnOwner = Cast<APawn>(Owner);
	if (PawnOwner)
	{
		// This can be false very early (pre-possession). Prefer controller-based checks when possible.
		if (PawnOwner->IsLocallyControlled())
		{
			return true;
		}

		if (const AController* Controller = PawnOwner->GetController())
		{
			return Controller->IsLocalController();
		}

		return false;
	}

	// Fallback: if attached to something else, treat as enabled only on non-dedicated.
	return (Owner->GetNetMode() != NM_DedicatedServer);
}

FVector UInteractableIconSelectorComponent::GetViewLocation() const
{
	FVector ViewLoc = FVector::ZeroVector;
	FRotator ViewRot = FRotator::ZeroRotator;

	if (const APawn* PawnOwner = Cast<APawn>(GetOwner()))
	{
		if (AController* Controller = PawnOwner->GetController())
		{
			Controller->GetPlayerViewPoint(ViewLoc, ViewRot);
			return ViewLoc;
		}
	}

	if (const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->GetPlayerViewPoint(ViewLoc, ViewRot);
	}
	return ViewLoc;
}

FRotator UInteractableIconSelectorComponent::GetViewRotation() const
{
	FVector ViewLoc = FVector::ZeroVector;
	FRotator ViewRot = FRotator::ZeroRotator;

	if (const APawn* PawnOwner = Cast<APawn>(GetOwner()))
	{
		if (AController* Controller = PawnOwner->GetController())
		{
			Controller->GetPlayerViewPoint(ViewLoc, ViewRot);
			return ViewRot;
		}
	}

	if (const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->GetPlayerViewPoint(ViewLoc, ViewRot);
	}
	return ViewRot;
}

bool UInteractableIconSelectorComponent::QueryNearbyCandidates(TArray<AActor*>& OutCandidates) const
{
	OutCandidates.Reset();

	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner)
	{
		return false;
	}

	const FVector Origin = GetViewLocation();

	TArray<AActor*> OverlappedActors;
	OverlappedActors.Reserve(MaxCandidatesToScore);

	// We use a generic object query here (WorldDynamic + WorldStatic) to catch most interactables.
	// For best results, define a custom ObjectType for interactables and use only that.
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(Owner);

	UKismetSystemLibrary::SphereOverlapActors(
		this,
		Origin,
		MaxScanDistance,
		ObjectTypes,
		AActor::StaticClass(),
		ActorsToIgnore,
		OverlappedActors);

	if (OverlappedActors.IsEmpty())
	{
		return false;
	}

	OutCandidates.Reserve(FMath::Min(MaxCandidatesToScore, OverlappedActors.Num()));

	for (AActor* Other : OverlappedActors)
	{
		if (!Other || Other == Owner)
		{
			continue;
		}

		if (!IsInteractableActor(Other))
		{
			continue;
		}

		OutCandidates.Add(Other);
		if (OutCandidates.Num() >= MaxCandidatesToScore)
		{
			break;
		}
	}

	return OutCandidates.Num() > 0;
}

FVector UInteractableIconSelectorComponent::GetCandidateIconLocation(AActor* Candidate) const
{
	return GetIconWorldLocationFromActor(Candidate);
}

bool UInteractableIconSelectorComponent::HasLineOfSightTo(AActor* Candidate, const FVector& ViewLocation, const FVector& TargetLocation) const
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner || !Candidate)
	{
		return false;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractableIconSelector_LOS), false);
	Params.AddIgnoredActor(Owner);

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, ViewLocation, TargetLocation, LineOfSightTraceChannel, Params);

	// Visible if we hit nothing, or we hit the candidate itself.
	return (!bHit) || (Hit.GetActor() == Candidate);
}

float UInteractableIconSelectorComponent::ScoreCandidate(AActor* Candidate, const FVector& ViewLocation, const FVector& ViewForward, const FVector2D& ViewportCenter, const FVector2D& ViewportSize, bool& bOutVisible) const
{
	LastRejectReason.Reset();

	bOutVisible = true;
	if (!Candidate)
	{
		LastRejectReason = TEXT("NullCandidate");
		return -1.0f;
	}

	if (!IsInteractableActor(Candidate))
	{
		LastRejectReason = TEXT("NotInteractable");
		return -1.0f;
	}

	const FVector TargetLoc = GetCandidateIconLocation(Candidate);
	const FVector ToTarget = (TargetLoc - ViewLocation);
	const float Dist = ToTarget.Size();

	const float MinDist = GetMinimumDistanceToShow(Candidate);
	if (Dist <= 1.0f)
	{
		LastRejectReason = TEXT("DistTooSmall");
		return -1.0f;
	}
	if (Dist > MaxScanDistance)
	{
		LastRejectReason = TEXT("BeyondMaxScanDistance");
		return -1.0f;
	}
	if (Dist < MinDist)
	{
		LastRejectReason = FString::Printf(TEXT("BelowMinDist(%.1f<%.1f)"), Dist, MinDist);
		return -1.0f;
	}

	const FVector Dir = ToTarget / Dist;
	const float ForwardDot = FVector::DotProduct(ViewForward, Dir); // -1..1
	if (ForwardDot < MinForwardDotToConsider)
	{
		LastRejectReason = FString::Printf(TEXT("ForwardDotTooLow(%.3f<%.3f)"), ForwardDot, MinForwardDotToConsider);
		return -1.0f;
	}

	// Distance score (0..1) where closer is better.
	const float NormalizedDist = FMath::Clamp(Dist / FMath::Max(MaxScanDistance, 1.0f), 0.0f, 1.0f);
	const float DistanceScore = 1.0f - NormalizedDist;

	// Visibility (LOS)
	bOutVisible = HasLineOfSightTo(Candidate, ViewLocation, TargetLoc);
	if (bRequireLineOfSight && !bOutVisible)
	{
		LastRejectReason = TEXT("NoLineOfSight");
		return -1.0f;
	}

	float PositionScore = 0.0f;

	if (PositionScoreMode == EInteractableIconPositionScoreMode::PawnForward)
	{
		// Optional off-screen rejection even in PawnForward mode.
		if (bRejectOffScreenInPawnForwardMode)
		{
			APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
			if (!PC)
			{
				LastRejectReason = TEXT("NoPlayerController");
				return -1.0f;
			}

			FVector2D ScreenPos(0.0f, 0.0f);
			const bool bProjected = PC->ProjectWorldLocationToScreen(TargetLoc, ScreenPos, true);
			if (!bProjected || ViewportSize.X <= 0.0f || ViewportSize.Y <= 0.0f)
			{
				LastRejectReason = FString::Printf(TEXT("ProjectFailOrBadViewport(Projected=%d VP=%.0fx%.0f)"), bProjected ? 1 : 0, ViewportSize.X, ViewportSize.Y);
				return -1.0f;
			}

			const bool bOnScreen = (ScreenPos.X >= 0.0f) && (ScreenPos.X <= ViewportSize.X) && (ScreenPos.Y >= 0.0f) && (ScreenPos.Y <= ViewportSize.Y);
			if (!bOnScreen)
			{
				LastRejectReason = FString::Printf(TEXT("OffScreen(Screen=%.0f,%.0f VP=%.0f,%.0f)"), ScreenPos.X, ScreenPos.Y, ViewportSize.X, ViewportSize.Y);
				return -1.0f;
			}
		}

		// Pure forward-based score. ForwardDot is already >= MinForwardDotToConsider.
		// Remap [-1..1] to [0..1] for nicer readability.
		PositionScore = (ForwardDot + 1.0f) * 0.5f;
	}
	else // ScreenCenter
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (!PC)
		{
			LastRejectReason = TEXT("NoPlayerController");
			return -1.0f;
		}

		// Project to screen and reject off-screen candidates.
		FVector2D ScreenPos(0.0f, 0.0f);
		const bool bProjected = PC->ProjectWorldLocationToScreen(TargetLoc, ScreenPos, true);
		if (!bProjected || ViewportSize.X <= 0.0f || ViewportSize.Y <= 0.0f)
		{
			LastRejectReason = FString::Printf(TEXT("ProjectFailOrBadViewport(Projected=%d VP=%.0fx%.0f)"), bProjected ? 1 : 0, ViewportSize.X, ViewportSize.Y);
			return -1.0f;
		}

		const bool bOnScreen = (ScreenPos.X >= 0.0f) && (ScreenPos.X <= ViewportSize.X) && (ScreenPos.Y >= 0.0f) && (ScreenPos.Y <= ViewportSize.Y);
		if (!bOnScreen)
		{
			LastRejectReason = FString::Printf(TEXT("OffScreen(Screen=%.0f,%.0f VP=%.0f,%.0f)"), ScreenPos.X, ScreenPos.Y, ViewportSize.X, ViewportSize.Y);
			return -1.0f;
		}

		// Screen center score (0..1)
		const float DistToCenter = FVector2D::Distance(ScreenPos, ViewportCenter);
		const float HalfDiag = 0.5f * FMath::Sqrt(ViewportSize.X * ViewportSize.X + ViewportSize.Y * ViewportSize.Y);
		const float NormalizedCenterDist = (HalfDiag > 0.0f) ? FMath::Clamp(DistToCenter / HalfDiag, 0.0f, 1.0f) : 1.0f;

		// Hard reject when too far from the center (makes icons drop quickly when player looks away).
		if (NormalizedCenterDist > ScreenCenterMaxNormalizedDistance)
		{
			LastRejectReason = FString::Printf(TEXT("TooFarFromCenter(%.3f>%.3f)"), NormalizedCenterDist, ScreenCenterMaxNormalizedDistance);
			return -1.0f;
		}

		PositionScore = 1.0f - NormalizedCenterDist;
	}

	// --- Priority: Visibility > Position > Distance ---
	const float VisibilityTerm = bOutVisible ? 1000.0f : 0.0f;
	const float PositionTerm = PositionScore * 100.0f;
	const float DistanceTerm = DistanceScore * 10.0f;

	return VisibilityTerm + PositionTerm + DistanceTerm;
}

void UInteractableIconSelectorComponent::ScanAndApply()
{
	if (!bSelectorEnabled || !IsLocallyControlledOwner())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	int32 DebugLogged = 0;

	// Collect as strong pointers for this scan only.
	TArray<AActor*> Candidates;
	QueryNearbyCandidates(Candidates);

	const FVector ViewLoc = GetViewLocation();
	const FVector ViewForward = GetViewRotation().Vector();

	FVector2D ViewportSize(0.0f, 0.0f);
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		int32 SizeX = 0;
		int32 SizeY = 0;
		PC->GetViewportSize(SizeX, SizeY);
		ViewportSize = FVector2D((float)SizeX, (float)SizeY);
	}
	const FVector2D ViewportCenter = ViewportSize * 0.5f;

	AActor* BestActor = nullptr;
	float BestScore = -1.0f;

	TArray<AActor*> ConsideredThisScan;
	ConsideredThisScan.Reserve(Candidates.Num());

	for (AActor* Candidate : Candidates)
	{
		bool bVisible = true;
		const float Score = ScoreCandidate(Candidate, ViewLoc, ViewForward, ViewportCenter, ViewportSize, bVisible);

		if (bDebugScoring && DebugLogged < DebugMaxCandidatesToLog)
		{
			FVector TargetLoc = Candidate ? GetCandidateIconLocation(Candidate) : FVector::ZeroVector;

			// Compute screen position for debug (best effort)
			FVector2D ScreenPos(0.0f, 0.0f);
			bool bProjected = false;
			if (PC)
			{
				bProjected = PC->ProjectWorldLocationToScreen(TargetLoc, ScreenPos, true);
			}

			UE_LOG(LogTemp, Warning, TEXT("[IconSelector] %s Score=%.2f Visible=%d Rej='%s' Dist=%.0f Screen=(%.0f,%.0f) Proj=%d"),
				*GetNameSafe(Candidate),
				Score,
				bVisible ? 1 : 0,
				*LastRejectReason,
				(FVector::Dist(ViewLoc, TargetLoc)),
				ScreenPos.X, ScreenPos.Y,
				bProjected ? 1 : 0);
			DebugLogged++;

			if (bDebugDraw)
			{
				const FString Text = FString::Printf(TEXT("%s\nS=%.1f V=%d\n%s"), *GetNameSafe(Candidate), Score, bVisible ? 1 : 0, *LastRejectReason);
				DrawDebugString(World, TargetLoc + FVector(0, 0, 30.0f), Text, nullptr, Score >= 0.0f ? FColor::Green : FColor::Red, DebugDrawDuration, false);
			}
		}

		if (Score < 0.0f)
		{
			SetIconVisibilityOnActor(Candidate, false);
			continue;
		}

		ConsideredThisScan.Add(Candidate);

		if (Score > BestScore)
		{
			BestScore = Score;
			BestActor = Candidate;
		}
	}

	if (bDebugScoring)
	{
		UE_LOG(LogTemp, Warning, TEXT("[IconSelector] Best=%s BestScore=%.2f Considered=%d TotalCandidates=%d"), *GetNameSafe(BestActor), BestScore, ConsideredThisScan.Num(), Candidates.Num());
	}

	// Hysteresis: avoid flicker when scores are close.
	if (CurrentBestActor.IsValid() && BestActor != CurrentBestActor.Get())
	{
		if (BestActor && BestScore < (CurrentBestScore + MinSwitchScoreDelta))
		{
			BestActor = CurrentBestActor.Get();
			BestScore = CurrentBestScore;
		}
	}

	ApplySelection(BestActor, ConsideredThisScan);

	// Hide anything that was previously considered but no longer is.
	for (const TWeakObjectPtr<AActor>& Prev : PreviouslyConsideredActors)
	{
		AActor* PrevActor = Prev.Get();
		if (!PrevActor)
		{
			continue;
		}

		if (!ConsideredThisScan.Contains(PrevActor))
		{
			SetIconVisibilityOnActor(PrevActor, false);
		}
	}

	PreviouslyConsideredActors.Reset(ConsideredThisScan.Num());
	for (AActor* Considered : ConsideredThisScan)
	{
		PreviouslyConsideredActors.Add(Considered);
	}

	CurrentBestActor = BestActor;
	CurrentBestScore = BestActor ? BestScore : -1.0f;
}

void UInteractableIconSelectorComponent::ApplySelection(AActor* NewBestActor, const TArray<AActor*>& ConsideredActors)
{
	for (AActor* Actor : ConsideredActors)
	{
		if (!Actor || !IsInteractableActor(Actor))
		{
			continue;
		}

		const bool bShouldBeVisible = (Actor == NewBestActor);
		SetIconVisibilityOnActor(Actor, bShouldBeVisible);
	}

	// Also make sure the old best gets hidden if it left the overlap list.
	if (AActor* OldBest = CurrentBestActor.Get())
	{
		if (OldBest != NewBestActor)
		{
			SetIconVisibilityOnActor(OldBest, false);
		}
	}
}

// ---- BPI reflection wrappers ----

bool UInteractableIconSelectorComponent::DoesActorImplementBPI(AActor* Actor) const
{
	return Actor && InteractableIconSelector::Private::FindFunc(Actor, BPI_SetIconVisibilityFuncName) != nullptr;
}

float UInteractableIconSelectorComponent::GetMinimumDistanceViaBPI(AActor* Actor) const
{
	if (!Actor)
	{
		return 0.0f;
	}

	UFunction* Func = InteractableIconSelector::Private::FindFunc(Actor, BPI_GetMinimumDistanceFuncName);
	if (!Func)
	{
		return 0.0f;
	}

	struct FParams
	{
		float ReturnValue = 0.0f;
	};

	FParams Params;
	Actor->ProcessEvent(Func, &Params);
	return Params.ReturnValue;
}

void UInteractableIconSelectorComponent::SetIconVisibilityViaBPI(AActor* Actor, bool bVisible) const
{
	if (!Actor)
	{
		return;
	}

	UFunction* Func = InteractableIconSelector::Private::FindFunc(Actor, BPI_SetIconVisibilityFuncName);
	if (!Func)
	{
		return;
	}

	struct FParams
	{
		bool bVisible = false;
	};

	FParams Params;
	Params.bVisible = bVisible;
	Actor->ProcessEvent(Func, &Params);
}

bool UInteractableIconSelectorComponent::GetIconWorldLocationViaBPI(AActor* Actor, FVector& OutLocation) const
{
	OutLocation = FVector::ZeroVector;
	if (!Actor)
	{
		return false;
	}

	UFunction* Func = InteractableIconSelector::Private::FindFunc(Actor, BPI_GetIconWorldLocationFuncName);
	if (!Func)
	{
		return false;
	}

	struct FParams
	{
		FVector ReturnValue = FVector::ZeroVector;
	};

	FParams Params;
	Actor->ProcessEvent(Func, &Params);
	OutLocation = Params.ReturnValue;
	return true;
}

bool UInteractableIconSelectorComponent::IsInteractableActor(AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	if (bUseBPIInteractable)
	{
		return DoesActorImplementBPI(Actor);
	}

	return Actor->Implements<UInteractableIconInterface>();
}

float UInteractableIconSelectorComponent::GetMinimumDistanceToShow(AActor* Actor) const
{
	if (!Actor)
	{
		return 0.0f;
	}

	if (bUseBPIInteractable)
	{
		return GetMinimumDistanceViaBPI(Actor);
	}

	return Actor->Implements<UInteractableIconInterface>()
		? IInteractableIconInterface::Execute_GetMinimumDistanceToShowIcon(Actor)
		: 0.0f;
}

void UInteractableIconSelectorComponent::SetIconVisibilityOnActor(AActor* Actor, bool bVisible) const
{
	if (!Actor)
	{
		return;
	}

	if (bUseBPIInteractable)
	{
		SetIconVisibilityViaBPI(Actor, bVisible);
		return;
	}

	if (Actor->Implements<UInteractableIconInterface>())
	{
		IInteractableIconInterface::Execute_SetIconVisibility(Actor, bVisible);
	}
}

FVector UInteractableIconSelectorComponent::GetIconWorldLocationFromActor(AActor* Actor) const
{
	if (!Actor)
	{
		return FVector::ZeroVector;
	}

	if (bUseBPIInteractable)
	{
		FVector Loc;
		if (GetIconWorldLocationViaBPI(Actor, Loc) && !Loc.IsNearlyZero())
		{
			return Loc;
		}
		return Actor->GetActorLocation();
	}

	if (Actor->Implements<UInteractableIconInterface>())
	{
		const FVector Loc = IInteractableIconInterface::Execute_GetIconWorldLocation(Actor);
		return Loc.IsNearlyZero() ? Actor->GetActorLocation() : Loc;
	}

	return Actor->GetActorLocation();
}
