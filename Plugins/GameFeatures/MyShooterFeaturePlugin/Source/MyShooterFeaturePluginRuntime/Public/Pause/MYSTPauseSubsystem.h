// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "MYSTPauseSubsystem.generated.h"

/**
 * Fired whenever the paused state changes.
 * Bind your pause-menu widget to this so it shows/hides automatically.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMYSTPauseStateChanged, bool, bIsPaused);

/**
 * UMYSTPauseSubsystem
 *
 * Single-player pause manager.  Call RequestPause / RequestUnpause / TogglePause
 * from Blueprint or C++.  Handles:
 *   - Engine world-pause via APlayerController::SetPause  (stops Tick, physics,
 *     timelines, AI, animations on all actors).
 *   - GAS ability-task suspension by disabling the ASC component tick on every
 *     pawn in the world (prevents ability tasks from running while paused).
 *
 * Blueprint usage:
 *   GetGameInstance → GetSubsystem(MYSTPauseSubsystem) → TogglePause
 *   Bind OnPauseStateChanged → drive your pause-menu widget visibility.
 */
UCLASS(meta = (DisplayName = "MYST Pause Subsystem"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API UMYSTPauseSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	// -----------------------------------------------------------------------
	// USubsystem interface
	// -----------------------------------------------------------------------

	/** Always create — subsystem is lightweight and needed from the first map. */
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	// -----------------------------------------------------------------------
	// Delegates
	// -----------------------------------------------------------------------

	/**
	 * Broadcast whenever the paused state flips.
	 * Payload is true when the game just became paused, false when unpaused.
	 */
	UPROPERTY(BlueprintAssignable, Category = "MYST|Pause")
	FMYSTPauseStateChanged OnPauseStateChanged;

	// -----------------------------------------------------------------------
	// Public API
	// -----------------------------------------------------------------------

	/** Pause the game.  No-op if already paused. */
	UFUNCTION(BlueprintCallable, Category = "MYST|Pause")
	void RequestPause();

	/** Unpause the game.  No-op if not paused. */
	UFUNCTION(BlueprintCallable, Category = "MYST|Pause")
	void RequestUnpause();

	/** Toggle between paused and unpaused. */
	UFUNCTION(BlueprintCallable, Category = "MYST|Pause")
	void TogglePause();

	/** Returns true while the game is paused. */
	UFUNCTION(BlueprintPure, Category = "MYST|Pause")
	bool IsPaused() const { return bCurrentlyPaused; }

private:

	/**
	 * Core implementation.  Calls APlayerController::SetPause and adjusts
	 * ASC component ticks on all pawns.
	 */
	void ApplyPause(bool bPause);

	bool bCurrentlyPaused = false;
};

