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

	// Store surface data — never updated from animations.
	CoverSurfaceNormal  = ResolvedNormal;
	CoverSurfaceTangent = FVector::CrossProduct(FVector::UpVector, ResolvedNormal).GetSafeNormal();
	CoverMaxMoveSpeed   = MaxSpeed;
	CoverComponent      = ResolvedComponent;

	// Derive the perpendicular distance from the character centre to the wall plane.
	CoverDistanceFromWall = FVector::DotProduct(
		CharacterOwner->GetActorLocation() - ResolvedImpactPoint, ResolvedNormal);

	// Rotate character to face away from the cover wall.
	const FRotator FaceAwayFromWall = UKismetMathLibrary::MakeRotFromX(-CoverSurfaceNormal);
	CharacterOwner->SetActorRotation(FaceAwayFromWall);

	// Switch movement mode — triggers Lyra's OnMovementModeChanged → SetMovementModeTag
	// which fires the "Movement.Mode.Cover" GAS tag on the ASC automatically.
	SetMovementMode(MOVE_Custom, COVER_CUSTOM_MODE);
}

void ULyraCharacterMovementComponent::ExitCoverMode()
{
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
	// 3. GRAVITY + FLOOR: integrate Z velocity and sweep downward so the
	//    character stays grounded without reimplementing PhysWalking.
	// -----------------------------------------------------------------------
	const float PrevVelZ = Velocity.Z;
	Velocity.Z += GetGravityZ() * DeltaTime;
	const float AvgVelZ  = 0.5f * (PrevVelZ + Velocity.Z);
	const float ZDelta   = AvgVelZ * DeltaTime;

	// -----------------------------------------------------------------------
	// 4. MOVE: apply the combined XY snap + Z gravity in one swept move.
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
}

