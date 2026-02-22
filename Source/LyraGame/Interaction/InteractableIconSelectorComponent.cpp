// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction/InteractableIconSelectorComponent.h"

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
#include "Components/ActorComponent.h"

// UE_LOG formatting utilities in UE 5.7 rely on some std type-traits.
#include "Templates/IsArithmetic.h" // ensures UE toolchain pulls in required type-traits
#include "Templates/UnrealTemplate.h" // pull in UE's std type-traits bridges for log formatting (remove_reference_t)

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
		SetWidgetVisibilityOnActor(Best, false, 0.0f);
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
		if (AActor* Best = CurrentBestActor.Get())
		{
			SetWidgetVisibilityOnActor(Best, false, 0.0f);
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

	if (bDebugScoring)
	{
		UE_LOG(LogTemp, Warning, TEXT("[IconSelector] Overlap returned %d actors (Origin=%s Radius=%.0f)"), OverlappedActors.Num(), *Origin.ToString(), MaxScanDistance);
	}

	// ...existing code...
	for (AActor* Other : OverlappedActors)
	{
		if (!Other || Other == Owner)
		{
			continue;
		}

		if (!IsInteractableActor(Other))
		{
			if (bDebugScoring)
			{
				const bool bBPI = DoesActorImplementBPI(Other);
				const bool bHasComp = (FindInteractableComponent(Other) != nullptr);
				UE_LOG(LogTemp, Warning, TEXT("[IconSelector] Reject %s BPI=%d HasInteractableComp=%d"), *GetNameSafe(Other), bBPI ? 1 : 0, bHasComp ? 1 : 0);
			}
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

FVector UInteractableIconSelectorComponent::GetIconWorldLocationFromActor(AActor* Actor) const
{
	if (!Actor)
	{
		return FVector::ZeroVector;
	}

	FVector OutLoc = FVector::ZeroVector;
	if (GetIconWorldLocationViaBPI(Actor, OutLoc))
	{
		return OutLoc;
	}

	return Actor->GetActorLocation();
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

	// NOTE: In UE 5.7 FVector is typically double-precision (TVector<double>) and its operator/
	// has constraints that can reject arithmetic types depending on toolchain settings.
	// GetSafeNormal is the most robust/idiomatic way to normalize.
	const FVector Dir = ToTarget.GetSafeNormal();
	const float ForwardDot = (float)FVector::DotProduct(ViewForward, Dir); // -1..1
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

	float PositionScore;

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
	float BestDistance = TNumericLimits<float>::Max();

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
			SetWidgetVisibilityOnActor(Candidate, false, 0.0f);
			continue;
		}

		ConsideredThisScan.Add(Candidate);

		const float CandidateDistance = FVector::Dist(ViewLoc, GetCandidateIconLocation(Candidate));

		// Primary: higher score wins. Tie-breaker: closer wins.
		if ((Score > BestScore) || (FMath::IsNearlyEqual(Score, BestScore, 0.001f) && (CandidateDistance < BestDistance)))
		{
			BestScore = Score;
			BestActor = Candidate;
			BestDistance = CandidateDistance;
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

	// If nothing is selected now, ensure we hide the previous best immediately.
	if (!BestActor)
	{
		if (AActor* OldBest = CurrentBestActor.Get())
		{
			SetWidgetVisibilityOnActor(OldBest, false, 0.0f);
		}
	}

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
			SetWidgetVisibilityOnActor(PrevActor, false, 0.0f);
		}
	}

	PreviouslyConsideredActors.Reset(ConsideredThisScan.Num());
	for (AActor* Considered : ConsideredThisScan)
	{
		TWeakObjectPtr<AActor> Weak(static_cast<AActor*>(Considered));
		PreviouslyConsideredActors.Add(Weak);
	}

	CurrentBestActor = TWeakObjectPtr<AActor>(static_cast<AActor*>(BestActor));
	CurrentBestScore = BestActor ? BestScore : -1.0f;
}

void UInteractableIconSelectorComponent::ApplySelection(AActor* NewBestActor, const TArray<AActor*>& ConsideredActors)
{
	// Compute view location once; used for distance.
	const FVector ViewLoc = GetViewLocation();

	for (AActor* Actor : ConsideredActors)
	{
		if (!Actor || !IsInteractableActor(Actor))
		{
			continue;
		}

		const bool bShouldBeVisible = (Actor == NewBestActor);
		if (bShouldBeVisible)
		{
			const FVector TargetLoc = GetCandidateIconLocation(Actor);
			const float PlayerDistance = FVector::Dist(ViewLoc, TargetLoc);
			SetWidgetVisibilityOnActor(Actor, true, PlayerDistance);
		}
		else
		{
			SetWidgetVisibilityOnActor(Actor, false, 0.0f);
		}
	}

	// Always hide the old best if it differs from the new best (including when NewBestActor is null).
	if (AActor* OldBest = CurrentBestActor.Get())
	{
		if (OldBest != NewBestActor)
		{
			SetWidgetVisibilityOnActor(OldBest, false, 0.0f);
		}
	}
}

// ---- BPI reflection wrappers ----

float UInteractableIconSelectorComponent::GetMinimumDistanceViaBPI(AActor* Actor) const
{
	if (!Actor)
	{
		return 0.0f;
	}

	UFunction* Func = InteractableIconSelector::Private::FindFunc(Actor, BPI_GetMinimumDistanceFuncName);
	if (!Func)
	{
		// Safe fallback.
		return 0.0f;
	}

	uint8* Buffer = (uint8*)FMemory_Alloca(Func->ParmsSize);
	FMemory::Memzero(Buffer, Func->ParmsSize);

	Actor->ProcessEvent(Func, Buffer);

	// Read return value as float.
	if (FProperty* ReturnProp = Func->GetReturnProperty())
	{
		if (FFloatProperty* FloatProp = CastField<FFloatProperty>(ReturnProp))
		{
			return FMath::Max(0.0f, FloatProp->GetPropertyValue_InContainer(Buffer));
		}
		if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(ReturnProp))
		{
			return (float)FMath::Max(0.0, DoubleProp->GetPropertyValue_InContainer(Buffer));
		}
	}

	if (FProperty* ReturnByName = Func->FindPropertyByName(TEXT("ReturnValue")))
	{
		if (FFloatProperty* FloatProp = CastField<FFloatProperty>(ReturnByName))
		{
			return FMath::Max(0.0f, FloatProp->GetPropertyValue_InContainer(Buffer));
		}
		if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(ReturnByName))
		{
			return (float)FMath::Max(0.0, DoubleProp->GetPropertyValue_InContainer(Buffer));
		}
	}

	return 0.0f;
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

	uint8* Buffer = (uint8*)FMemory_Alloca(Func->ParmsSize);
	FMemory::Memzero(Buffer, Func->ParmsSize);

	Actor->ProcessEvent(Func, Buffer);

	if (FProperty* ReturnProp = Func->GetReturnProperty())
	{
		if (FStructProperty* StructProp = CastField<FStructProperty>(ReturnProp))
		{
			if (StructProp->Struct == TBaseStructure<FVector>::Get())
			{
				OutLocation = *StructProp->ContainerPtrToValuePtr<FVector>(Buffer);
				return true;
			}
		}
	}

	if (FProperty* ReturnByName = Func->FindPropertyByName(TEXT("ReturnValue")))
	{
		if (FStructProperty* StructProp = CastField<FStructProperty>(ReturnByName))
		{
			if (StructProp->Struct == TBaseStructure<FVector>::Get())
			{
				OutLocation = *StructProp->ContainerPtrToValuePtr<FVector>(Buffer);
				return true;
			}
		}
	}

	return false;
}

bool UInteractableIconSelectorComponent::DoesActorImplementBPI(AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	// Presence check: if the actor has a UFunction with this name, we treat it as implementing BPI_Interactable.
	return InteractableIconSelector::Private::FindFunc(Actor, BPI_InteractablePresenceFuncName) != nullptr;
}

UActorComponent* UInteractableIconSelectorComponent::FindInteractableComponent(AActor* Actor) const
{
	if (!Actor)
	{
		return nullptr;
	}

	// Preferred: exact class configured (BPAC_Interactable).
	UClass* DesiredClass = InteractableComponentClass.LoadSynchronous();
	if (DesiredClass)
	{
		return Actor->GetComponentByClass(DesiredClass);
	}

	// Fallback: if not configured, scan components for a SetWidgetVisibility function.
	if (bAllowAnyComponentWithSetWidgetVisibility)
	{
		static const FName FuncName_SetWidgetVisibility(TEXT("SetWidgetVisibility"));
		TInlineComponentArray<UActorComponent*> Components;
		Actor->GetComponents(Components);
		for (UActorComponent* Comp : Components)
		{
			if (Comp && InteractableIconSelector::Private::FindFunc(Comp, FuncName_SetWidgetVisibility))
			{
				return Comp;
			}
		}
	}

	return nullptr;
}

void UInteractableIconSelectorComponent::SetWidgetVisibilityOnActor(AActor* Actor, bool bVisible, float PlayerDistance) const
{
	if (!Actor)
	{
		return;
	}

	UActorComponent* InteractableComp = FindInteractableComponent(Actor);
	if (!InteractableComp)
	{
		return;
	}

	// Call BPAC_Interactable::SetWidgetVisibility(bool Visible, float Distance) via reflection.
	static const FName FuncName_SetWidgetVisibility(TEXT("SetWidgetVisibility"));
	UFunction* Func = InteractableIconSelector::Private::FindFunc(InteractableComp, FuncName_SetWidgetVisibility);
	if (!Func)
	{
		return;
	}

	uint8* Buffer = (uint8*)FMemory_Alloca(Func->ParmsSize);
	FMemory::Memzero(Buffer, Func->ParmsSize);

	// Prefer the exact parameter names used by the Blueprint component.
	FBoolProperty* BoolProp = nullptr;
	FProperty* DistProp = nullptr;

	if (FProperty* NamedVisible = Func->FindPropertyByName(TEXT("Visible")))
	{
		BoolProp = CastField<FBoolProperty>(NamedVisible);
	}
	if (!BoolProp)
	{
		if (FProperty* NamedVisible2 = Func->FindPropertyByName(TEXT("bVisible")))
		{
			BoolProp = CastField<FBoolProperty>(NamedVisible2);
		}
	}
	if (!BoolProp)
	{
		if (FProperty* NamedVisible3 = Func->FindPropertyByName(TEXT("visible")))
		{
			BoolProp = CastField<FBoolProperty>(NamedVisible3);
		}
	}

	// Distance can be float OR double depending on Large World Coordinates / Blueprint compilation.
	DistProp = Func->FindPropertyByName(TEXT("Distance"));
	if (!DistProp)
	{
		DistProp = Func->FindPropertyByName(TEXT("PlayerDistance"));
	}
	if (!DistProp)
	{
		DistProp = Func->FindPropertyByName(TEXT("distance"));
	}

	// Fallback by walking parameters in-order.
	for (TFieldIterator<FProperty> It(Func); It; ++It)
	{
		FProperty* Prop = *It;
		if (!Prop || !(Prop->PropertyFlags & CPF_Parm) || (Prop->PropertyFlags & CPF_ReturnParm))
		{
			continue;
		}

		if (!BoolProp)
		{
			BoolProp = CastField<FBoolProperty>(Prop);
			if (BoolProp)
			{
				continue;
			}
		}

		if (!DistProp)
		{
			if (Prop->IsA<FFloatProperty>() || Prop->IsA<FDoubleProperty>())
			{
				DistProp = Prop;
			}
		}

		if (BoolProp && DistProp)
		{
			break;
		}
	}

	if (!BoolProp || !DistProp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[IconSelector] SetWidgetVisibility param bind failed on %s (Bool=%d Dist=%d)"), *GetNameSafe(InteractableComp), BoolProp ? 1 : 0, DistProp ? 1 : 0);
	}

	if (BoolProp)
	{
		BoolProp->SetPropertyValue_InContainer(Buffer, bVisible);
	}

	if (DistProp)
	{
		const float DistanceToPass = bVisible ? PlayerDistance : 0.0f;
		if (FFloatProperty* FloatProp = CastField<FFloatProperty>(DistProp))
		{
			FloatProp->SetPropertyValue_InContainer(Buffer, DistanceToPass);
		}
		else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(DistProp))
		{
			DoubleProp->SetPropertyValue_InContainer(Buffer, (double)DistanceToPass);
		}
	}

	if (bDebugScoring && BoolProp && DistProp)
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("[IconSelector] SetWidgetVisibility bound params on %s: Visible=FBoolProperty, Distance=%s"), *GetNameSafe(InteractableComp), *DistProp->GetClass()->GetName());
	}

	InteractableComp->ProcessEvent(Func, Buffer);
}

bool UInteractableIconSelectorComponent::IsInteractableActor(AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	if (bRequireBPIInteractable && !DoesActorImplementBPI(Actor))
	{
		return false;
	}

	return FindInteractableComponent(Actor) != nullptr;
}

float UInteractableIconSelectorComponent::GetMinimumDistanceToShow(AActor* Actor) const
{
	return Actor ? GetMinimumDistanceViaBPI(Actor) : 0.0f;
}
