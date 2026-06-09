// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraCharacterMovementComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraCharacterMovementComponent)

UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_MovementStopped, "Gameplay.MovementStopped");

namespace LyraCharacter
{
	static float GroundTraceDistance = 100000.0f;
	FAutoConsoleVariableRef CVar_GroundTraceDistance(TEXT("LyraCharacter.GroundTraceDistance"), GroundTraceDistance, TEXT("Distance to trace down when generating ground information."), ECVF_Cheat);
};


ULyraCharacterMovementComponent::ULyraCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ULyraCharacterMovementComponent::SimulateMovement(float DeltaTime)
{
	if (bHasReplicatedAcceleration)
	{
		// Preserve our replicated acceleration
		const FVector OriginalAcceleration = Acceleration;
		Super::SimulateMovement(DeltaTime);
		Acceleration = OriginalAcceleration;
	}
	else
	{
		Super::SimulateMovement(DeltaTime);
	}
}

bool ULyraCharacterMovementComponent::CanAttemptJump() const
{
	// Same as UCharacterMovementComponent's implementation but without the crouch check
	return IsJumpAllowed() &&
		(IsMovingOnGround() || IsFalling()); // Falling included for double-jump and non-zero jump hold time, but validated by character.
}

void ULyraCharacterMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

const FLyraCharacterGroundInfo& ULyraCharacterMovementComponent::GetGroundInfo()
{
	if (!CharacterOwner || (GFrameCounter == CachedGroundInfo.LastUpdateFrame))
	{
		return CachedGroundInfo;
	}

	if (MovementMode == MOVE_Walking)
	{
		CachedGroundInfo.GroundHitResult = CurrentFloor.HitResult;
		CachedGroundInfo.GroundDistance = 0.0f;
	}
	else
	{
		const UCapsuleComponent* CapsuleComp = CharacterOwner->GetCapsuleComponent();
		check(CapsuleComp);

		const float CapsuleHalfHeight = CapsuleComp->GetUnscaledCapsuleHalfHeight();
		const ECollisionChannel CollisionChannel = (UpdatedComponent ? UpdatedComponent->GetCollisionObjectType() : ECC_Pawn);
		const FVector TraceStart(GetActorLocation());
		const FVector TraceEnd(TraceStart.X, TraceStart.Y, (TraceStart.Z - LyraCharacter::GroundTraceDistance - CapsuleHalfHeight));

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LyraCharacterMovementComponent_GetGroundInfo), false, CharacterOwner);
		FCollisionResponseParams ResponseParam;
		InitCollisionParams(QueryParams, ResponseParam);

		FHitResult HitResult;
		GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, CollisionChannel, QueryParams, ResponseParam);

		CachedGroundInfo.GroundHitResult = HitResult;
		CachedGroundInfo.GroundDistance = LyraCharacter::GroundTraceDistance;

		if (MovementMode == MOVE_NavWalking)
		{
			CachedGroundInfo.GroundDistance = 0.0f;
		}
		else if (HitResult.bBlockingHit)
		{
			CachedGroundInfo.GroundDistance = FMath::Max((HitResult.Distance - CapsuleHalfHeight), 0.0f);
		}
	}

	CachedGroundInfo.LastUpdateFrame = GFrameCounter;

	return CachedGroundInfo;
}

void ULyraCharacterMovementComponent::SetReplicatedAcceleration(const FVector& InAcceleration)
{
	bHasReplicatedAcceleration = true;
	Acceleration = InAcceleration;
}

FRotator ULyraCharacterMovementComponent::GetDeltaRotation(float DeltaTime) const
{
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		if (ASC->HasMatchingGameplayTag(TAG_Gameplay_MovementStopped))
		{
			return FRotator(0,0,0);
		}
	}

	return Super::GetDeltaRotation(DeltaTime);
}

float ULyraCharacterMovementComponent::GetMaxSpeed() const
{
	// Cover mode has its own speed; check before the Gameplay tag path.
	if (IsInCoverMode())
	{
		return CoverMaxMoveSpeed;
	}

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		if (ASC->HasMatchingGameplayTag(TAG_Gameplay_MovementStopped))
		{
			return 0;
		}
	}

	return Super::GetMaxSpeed();
}

// ---------------------------------------------------------------------------
// Cover Mode
// ---------------------------------------------------------------------------

void ULyraCharacterMovementComponent::EnterCoverMode(const FHitResult& WallHit, float MaxSpeed)
{
	if (!CharacterOwner)
	{
		return;
	}

	// Only allow entering cover while grounded — block if falling, swimming, flying, etc.
	if (MovementMode != MOVE_Walking)
	{
		return;
	}

	// Overlap events do not populate HitResult.Normal — it arrives as (0,0,0).
	// When that happens, fire a short line trace from the character toward the
	// overlapping component to recover a valid surface normal.
	FVector ResolvedNormal     = WallHit.Normal;
	FVector ResolvedImpactPoint = WallHit.ImpactPoint;
	UPrimitiveComponent* ResolvedComponent = WallHit.GetComponent();

	if (ResolvedNormal.IsNearlyZero() && ResolvedComponent && GetWorld())
	{
		const FVector CharLoc    = CharacterOwner->GetActorLocation();
		const FVector ComponentLoc = ResolvedComponent->GetComponentLocation();

		// Trace from the character toward the component origin; 300 cm is more than
		// enough for any typical cover-entry distance.
		const FVector TraceDir   = (ComponentLoc - CharLoc).GetSafeNormal();
		const FVector TraceStart = CharLoc;
		const FVector TraceEnd   = CharLoc + TraceDir * 300.0f;

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EnterCoverNormalRecover), false, CharacterOwner);
		FHitResult RecoveryHit;
		if (GetWorld()->LineTraceSingleByChannel(RecoveryHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams)
			&& !RecoveryHit.Normal.IsNearlyZero())
		{
			ResolvedNormal      = RecoveryHit.Normal;
			ResolvedImpactPoint = RecoveryHit.ImpactPoint;
			if (RecoveryHit.GetComponent())
			{
				ResolvedComponent = RecoveryHit.GetComponent();
			}
		}
		else
		{
			// Last resort: treat the vector FROM the component TO the character as the normal.
			ResolvedNormal      = (CharLoc - ComponentLoc).GetSafeNormal();
			ResolvedImpactPoint = ComponentLoc;
		}
	}

	// -----------------------------------------------------------------------
	// Surface width check — reject if the wall is narrower than the
	// character's back (~60 units).  Probe ±RequiredHalfWidth along the wall
	// tangent from the impact point; if either side finds no wall, or a
	// significantly different surface normal (e.g. a corner / thin edge),
	// the surface is too small to hide behind.
	// -----------------------------------------------------------------------
	{
		constexpr float RequiredHalfWidth = 30.0f;  // half of 60-unit minimum
		const FVector LocalTangent = FVector::CrossProduct(FVector::UpVector, ResolvedNormal).GetSafeNormal();

		const FVector ProbeOrigins[2] = {
			ResolvedImpactPoint - LocalTangent * RequiredHalfWidth,
			ResolvedImpactPoint + LocalTangent * RequiredHalfWidth
		};

		/*for (const FVector& ProbeOrigin : ProbeOrigins)
		{
			// Trace through the wall plane at this offset point.
			const FVector ProbeStart = ProbeOrigin + ResolvedNormal * 80.0f;
			const FVector ProbeEnd   = ProbeOrigin - ResolvedNormal * 80.0f;

			FCollisionQueryParams ProbeParams(SCENE_QUERY_STAT(EnterCoverWidthCheck), false, CharacterOwner);
			FHitResult ProbeHit;
			if (!GetWorld()->LineTraceSingleByChannel(ProbeHit, ProbeStart, ProbeEnd, ECC_Visibility, ProbeParams)
				|| FVector::DotProduct(ProbeHit.Normal, ResolvedNormal) < 0.7f)
			{
				// Surface edge or corner within required width — too narrow.
				return;
			}
		}*/
	}

	// Store surface data — never updated from animations.
	CoverSurfaceNormal  = ResolvedNormal;
	CoverSurfaceTangent = FVector::CrossProduct(FVector::UpVector, ResolvedNormal).GetSafeNormal();
	CoverMaxMoveSpeed   = MaxSpeed;
	CoverComponent      = ResolvedComponent;

	// Derive the perpendicular distance from the character centre to the wall plane.
	CoverDistanceFromWall = FVector::DotProduct(
		CharacterOwner->GetActorLocation() - ResolvedImpactPoint, ResolvedNormal);

	// -----------------------------------------------------------------------
	// ENTRY POSITION CORRECTION
	// If the character approached the wall at an angle the capsule may be at
	// the wrong perpendicular distance.  Project the current XY location onto
	// the wall plane and place the character exactly CoverDistanceFromWall away.
	// This eliminates the one-frame positional jitter on sideways entries.
	// -----------------------------------------------------------------------
	{
		const FVector CharLoc       = CharacterOwner->GetActorLocation();
		const float   CurrentPerp   = FVector::DotProduct(CharLoc - ResolvedImpactPoint, ResolvedNormal);
		const float   PerpError     = CurrentPerp - CoverDistanceFromWall;
		const FVector CorrectedLoc  = CharLoc - ResolvedNormal * PerpError;
		CharacterOwner->SetActorLocation(CorrectedLoc, false, nullptr, ETeleportType::TeleportPhysics);
	}

	// -----------------------------------------------------------------------
	// ROTATION LOCK SETUP
	// Disable controller-yaw so the camera can rotate freely while PhysCustom
	// enforces the wall-perpendicular yaw each tick.  Cached so ExitCoverMode
	// can restore the original value rather than hard-coding true/false.
	// -----------------------------------------------------------------------
	bCachedControllerRotationYaw         = CharacterOwner->bUseControllerRotationYaw;
	CharacterOwner->bUseControllerRotationYaw = false;

	// Snap rotation immediately so the character's back faces the wall on the
	// exact frame cover mode is entered (PhysCustom enforces it every tick after).
	const FRotator FaceAwayFromWall = UKismetMathLibrary::MakeRotFromX(-CoverSurfaceNormal);
	CharacterOwner->SetActorRotation(FaceAwayFromWall);

	// Switch movement mode — triggers Lyra's OnMovementModeChanged → SetMovementModeTag
	// which fires the "Movement.Mode.Cover" GAS tag on the ASC automatically.
	SetMovementMode(MOVE_Custom, COVER_CUSTOM_MODE);
}

void ULyraCharacterMovementComponent::ExitCoverMode()
{
	// Restore controller-yaw rotation to whatever it was before cover was entered.
	if (CharacterOwner)
	{
		CharacterOwner->bUseControllerRotationYaw = bCachedControllerRotationYaw;
	}

	CoverSurfaceNormal    = FVector::ZeroVector;
	CoverSurfaceTangent   = FVector::ZeroVector;
	CoverDistanceFromWall = 50.0f;
	CoverMaxMoveSpeed     = 250.0f;
	CoverComponent        = nullptr;

	// Restores MOVE_Walking — tag wiring removes "Movement.Mode.Cover" from ASC.
	SetMovementMode(MOVE_Walking);
}

bool ULyraCharacterMovementComponent::IsInCoverMode() const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == COVER_CUSTOM_MODE;
}

bool ULyraCharacterMovementComponent::CanCrouchInCurrentState() const
{
	// Cover mode is a grounded state — keep crouching legal so the engine's
	// UpdateCharacterStateBeforeMovement doesn't force-uncrouch every tick
	// (base impl returns false for any MOVE_Custom mode).
	if (IsInCoverMode())
	{
		return CanEverCrouch() && UpdatedComponent && !UpdatedComponent->IsSimulatingPhysics();
	}
	return Super::CanCrouchInCurrentState();
}

void ULyraCharacterMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	Super::PhysCustom(DeltaTime, Iterations);

	// Only handle our cover mode; ignore other potential future custom modes.
	if (CustomMovementMode != COVER_CUSTOM_MODE)
	{
		return;
	}

	if (DeltaTime < MIN_TICK_TIME || !CharacterOwner || CoverSurfaceTangent.IsNearlyZero())
	{
		return;
	}

	// -----------------------------------------------------------------------
	// 1. LATERAL: project input acceleration onto the stored tangent axis.
	//    Acceleration is populated from Add Movement Input before PhysCustom runs.
	// -----------------------------------------------------------------------
	const float LateralScale = FVector::DotProduct(Acceleration, CoverSurfaceTangent);
	// Map to [-1, 1] relative to MaxAcceleration, then scale by CoverMaxMoveSpeed.
	const float LateralSpeed = FMath::Clamp(LateralScale / FMath::Max(GetMaxAcceleration(), 1.0f), -1.0f, 1.0f)
	                           * CoverMaxMoveSpeed;

	Velocity   = CoverSurfaceTangent * LateralSpeed;
	Velocity.Z = 0.0f;  // Vertical handled separately below.

	// -----------------------------------------------------------------------
	// 2. PERPENDICULAR SNAP: from the desired lateral position, trace toward
	//    the wall and snap to exactly CoverDistanceFromWall.
	// -----------------------------------------------------------------------
	const FVector CurrentLocation = UpdatedComponent->GetComponentLocation();
	const FVector LateralDelta    = FVector(Velocity.X, Velocity.Y, 0.0f) * DeltaTime;
	const FVector ProbeOriginXY   = FVector(CurrentLocation.X + LateralDelta.X,
	                                        CurrentLocation.Y + LateralDelta.Y,
	                                        CurrentLocation.Z);

	const FVector TraceStart = ProbeOriginXY + CoverSurfaceNormal * 200.0f;
	const FVector TraceEnd   = ProbeOriginXY - CoverSurfaceNormal * 200.0f;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CoverPhysCustomSnap), false, CharacterOwner);
	FHitResult WallHit;
	const bool bHitWall = GetWorld()->LineTraceSingleByChannel(
		WallHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

	FVector TargetXY = ProbeOriginXY;
	if (bHitWall && FVector::DotProduct(WallHit.Normal, CoverSurfaceNormal) > 0.7f)
	{
		// Good wall hit — snap perpendicular distance.
		const FVector SnapPoint = WallHit.ImpactPoint + WallHit.Normal * CoverDistanceFromWall;
		TargetXY = FVector(SnapPoint.X, SnapPoint.Y, CurrentLocation.Z);

		// Keep the stored normal fresh for curved surfaces.
		CoverSurfaceNormal  = WallHit.Normal;
		CoverSurfaceTangent = FVector::CrossProduct(FVector::UpVector, CoverSurfaceNormal).GetSafeNormal();
	}
	else
	{
		// Lost contact with cover (walked off edge or obstruction).
		ExitCoverMode();
		return;
	}

	// -----------------------------------------------------------------------
	// 3. FLOOR SNAP — cover is always a grounded mode.
	// Sweep the full capsule straight down and snap to the floor in one frame.
	// This replaces gradual gravity accumulation, which causes a multi-frame
	// slow-float whenever the capsule is resized (e.g. crouching while in
	// cover) because MOVE_Custom has no PhysWalking instant floor-snap logic.
	// Gravity is used only as a fallback when no walkable floor is found
	// (e.g. the character walks off a ledge and cover is about to be lost).
	// -----------------------------------------------------------------------
	const UCapsuleComponent* CapsuleComp = CharacterOwner->GetCapsuleComponent();
	check(CapsuleComp);

	const FCollisionShape SnapCapsule = FCollisionShape::MakeCapsule(
		CapsuleComp->GetScaledCapsuleRadius(),
		CapsuleComp->GetScaledCapsuleHalfHeight());

	// 150 cm probe: covers the max standing→crouched capsule resize (~25 cm)
	// plus a comfortable margin for steps and slight airborne cases.
	const FVector SnapStart = FVector(TargetXY.X, TargetXY.Y, CurrentLocation.Z);
	const FVector SnapEnd   = SnapStart - FVector(0.f, 0.f, 150.f);

	FCollisionQueryParams SnapQueryParams(SCENE_QUERY_STAT(CoverPhysFloorSnap), false, CharacterOwner);
	FCollisionResponseParams SnapResponseParams;
	InitCollisionParams(SnapQueryParams, SnapResponseParams);

	FHitResult FloorSnapHit;
	const bool bFoundWalkableFloor = GetWorld()->SweepSingleByChannel(
		FloorSnapHit, SnapStart, SnapEnd, FQuat::Identity,
		UpdatedComponent->GetCollisionObjectType(),
		SnapCapsule, SnapQueryParams, SnapResponseParams);

	float ZDelta;
	if (bFoundWalkableFloor && FloorSnapHit.bBlockingHit
		&& !FloorSnapHit.bStartPenetrating
		&& FloorSnapHit.Normal.Z >= GetWalkableFloorZ())
	{
		// Snap directly to the floor — single-frame, no multi-frame settling.
		ZDelta     = -FloorSnapHit.Distance;
		Velocity.Z = 0.0f;
	}
	else
	{
		// No walkable floor within probe range — fall via gravity so the character
		// can drop off ledges naturally when cover geometry ends.
		const float PrevVelZ = Velocity.Z;
		Velocity.Z += GetGravityZ() * DeltaTime;
		ZDelta = 0.5f * (PrevVelZ + Velocity.Z) * DeltaTime;
	}

	// -----------------------------------------------------------------------
	// 4. MOVE: apply the combined XY snap + Z correction in one swept move.
	// -----------------------------------------------------------------------
	const FVector MoveDelta = (TargetXY - CurrentLocation) + FVector(0.0f, 0.0f, ZDelta);

	FHitResult MoveHit;
	SafeMoveUpdatedComponent(MoveDelta, UpdatedComponent->GetComponentQuat(), true, MoveHit);

	if (MoveHit.bBlockingHit)
	{
		if (MoveHit.Normal.Z > GetWalkableFloorZ())
		{
			// Landed on floor — zero vertical velocity.
			Velocity.Z = 0.0f;
		}

		HandleImpact(MoveHit, DeltaTime, MoveDelta);

		// Let the character slide along walls/corners without exiting cover.
		SlideAlongSurface(MoveDelta, 1.0f - MoveHit.Time, MoveHit.Normal, MoveHit, true);
	}

	// -----------------------------------------------------------------------
	// 5. ROTATION LOCK: enforce wall-perpendicular yaw every tick.
	//    CoverSurfaceNormal is already refreshed from the live wall trace above
	//    (step 2), so curved surfaces are handled automatically.
	//    Only Yaw is forced — Pitch and Roll remain at their CMC defaults (0).
	//
	//    EXCEPTION: skip the lock when ADSing or leaning — in those states the
	//    character needs to rotate freely with the camera for aiming.
	// -----------------------------------------------------------------------
	if (CharacterOwner && !CoverSurfaceNormal.IsNearlyZero())
	{
		bool bShouldLockYaw = true;

		// Check for ADS or lean tags — if any are present, allow free rotation.
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(CharacterOwner))
		{
			const FGameplayTag ADS_Tag          = FGameplayTag::RequestGameplayTag(FName("Event.Movement.ADS"));
			const FGameplayTag Lean_Generic_Tag = FGameplayTag::RequestGameplayTag(FName("Status.Cover.CanLean"));
			const FGameplayTag Lean_Left_Tag    = FGameplayTag::RequestGameplayTag(FName("Status.Cover.CanLeanLeft"));
			const FGameplayTag Lean_Right_Tag   = FGameplayTag::RequestGameplayTag(FName("Status.Cover.CanLeanRight"));

			if (ASC->HasMatchingGameplayTag(ADS_Tag)
				|| ASC->HasMatchingGameplayTag(Lean_Generic_Tag)
				|| ASC->HasMatchingGameplayTag(Lean_Left_Tag)
				|| ASC->HasMatchingGameplayTag(Lean_Right_Tag))
			{
				bShouldLockYaw = false;
			}
		}

		if (bShouldLockYaw)
		{
			const FRotator WallAwayRotation = UKismetMathLibrary::MakeRotFromX(-CoverSurfaceNormal);
			FRotator LockedRotation         = UpdatedComponent->GetComponentRotation();
			LockedRotation.Yaw              = WallAwayRotation.Yaw;
			UpdatedComponent->SetWorldRotation(LockedRotation);
		}
	}
}

