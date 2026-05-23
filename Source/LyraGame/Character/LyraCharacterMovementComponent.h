// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "NativeGameplayTags.h"

#include "LyraCharacterMovementComponent.generated.h"

class UObject;
struct FFrame;

LYRAGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_MovementStopped);

/**
 * FLyraCharacterGroundInfo
 *
 *	Information about the ground under the character.  It only gets updated as needed.
 */
USTRUCT(BlueprintType)
struct FLyraCharacterGroundInfo
{
	GENERATED_BODY()

	FLyraCharacterGroundInfo()
		: LastUpdateFrame(0)
		, GroundDistance(0.0f)
	{}

	uint64 LastUpdateFrame;

	UPROPERTY(BlueprintReadOnly)
	FHitResult GroundHitResult;

	UPROPERTY(BlueprintReadOnly)
	float GroundDistance;
};


/**
 * ULyraCharacterMovementComponent
 *
 *	The base character movement component class used by this project.
 */
UCLASS(Config = Game)
class LYRAGAME_API ULyraCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:

	ULyraCharacterMovementComponent(const FObjectInitializer& ObjectInitializer);

	virtual void SimulateMovement(float DeltaTime) override;

	virtual bool CanAttemptJump() const override;

	// Returns the current ground info.  Calling this will update the ground info if it's out of date.
	UFUNCTION(BlueprintCallable, Category = "Lyra|CharacterMovement")
	const FLyraCharacterGroundInfo& GetGroundInfo();

	void SetReplicatedAcceleration(const FVector& InAcceleration);

	//~UMovementComponent interface
	virtual FRotator GetDeltaRotation(float DeltaTime) const override;
	virtual float GetMaxSpeed() const override;
	//~End of UMovementComponent interface

	// --- Cover Mode constant ---
public:
	/** Index used for MOVE_Custom when in cover. Matches the entry in CustomMovementModeTagMap. */
	static constexpr uint8 COVER_CUSTOM_MODE = 0;

	// --- Cover mode API ---
public:
	/**
	 * Enters cover mode using the provided wall hit result.
	 * Stores surface data once; PhysCustom uses it every step.
	 * The perpendicular distance to the wall is derived from the hit result automatically.
	 * @param WallHit   The initial hit result from the cover detection trace/overlap.
	 * @param MaxSpeed  Lateral movement speed along cover (cm/s).
	 */
	UFUNCTION(BlueprintCallable, Category = "Lyra|CharacterMovement|Cover")
	void EnterCoverMode(const FHitResult& WallHit, float MaxSpeed);

	/** Exits cover mode and restores MOVE_Walking. */
	UFUNCTION(BlueprintCallable, Category = "Lyra|CharacterMovement|Cover")
	void ExitCoverMode();

	/** Returns true if currently in the cover custom movement mode. */
	UFUNCTION(BlueprintPure, Category = "Lyra|CharacterMovement|Cover")
	bool IsInCoverMode() const;

	// --- Cover state (readable from BP for lean/edge traces) ---
public:
	/** Wall surface normal, captured once on EnterCoverMode. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Cover State")
	FVector CoverSurfaceNormal = FVector::ZeroVector;

	/**
	 * Lateral axis along the wall surface.
	 * Computed as Cross(UpVector, CoverSurfaceNormal) on entry.
	 * Use this as the trace direction for lean/edge detection instead of shoulder sockets.
	 */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Cover State")
	FVector CoverSurfaceTangent = FVector::ZeroVector;

	/** Target perpendicular distance from wall surface (cm). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Cover State")
	float CoverDistanceFromWall = 50.0f;

	/** Maximum lateral speed while in cover (cm/s). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Cover State")
	float CoverMaxMoveSpeed = 250.0f;

	/** The primitive component whose surface we are attached to. Used to validate wall snaps. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Cover State")
	TObjectPtr<UPrimitiveComponent> CoverComponent = nullptr;

protected:

	virtual void InitializeComponent() override;

	// --- CMC overrides ---
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;

protected:

	// Cached ground info for the character.  Do not access this directly!  It's only updated when accessed via GetGroundInfo().
	FLyraCharacterGroundInfo CachedGroundInfo;

	UPROPERTY(Transient)
	bool bHasReplicatedAcceleration = false;
};
