// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "LoadingProcessInterface.h"

#include "MYSTLevelLoadingSubsystem.generated.h"

class ULoadingScreenManager;
class UWorld;

// ---------------------------------------------------------------------------
// Phase enum — tracks where in the load cycle we are
// ---------------------------------------------------------------------------

/**
 * Internal phase of the level loading pipeline.
 * Exposed to Blueprint so progress widgets and debug displays can query it.
 */
UENUM(BlueprintType)
enum class EMYSTLevelLoadPhase : uint8
{
	/** Not loading anything. Loading screen is hidden. */
	Idle				UMETA(DisplayName = "Idle"),

	/** OpenLevel was called; waiting for the new map to finish initial load. */
	WaitingForMap		UMETA(DisplayName = "Waiting For Map"),

	/** Map is loaded; waiting for all streaming levels / World Partition cells to become visible. */
	WaitingForStreaming	UMETA(DisplayName = "Waiting For Streaming"),

	/** Streaming is complete; waiting for PSO / shader precompilation to finish. */
	WaitingForPSO		UMETA(DisplayName = "Waiting For PSO"),
};

// ---------------------------------------------------------------------------
// Delegate payload structs
// ---------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMYSTLoadingSimpleDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMYSTShaderProgressDelegate, float, NormalizedProgress, int32, RemainingShadersCount);

// ---------------------------------------------------------------------------
// UMYSTLevelLoadingSubsystem
// ---------------------------------------------------------------------------

/**
 * Single-player level loading subsystem.
 *
 * Call RequestLevelTravel() instead of OpenLevel directly. This subsystem:
 *   1. Registers itself with the CommonLoadingScreen manager so the loading
 *      screen appears immediately and does NOT hide until all phases pass.
 *   2. Waits for all streaming levels / World Partition cells to become visible.
 *   3. Waits for PSO (pipeline state object) shader precompilation to drain.
 *   4. Broadcasts Blueprint-assignable delegates at each phase transition.
 *
 * The loading screen is driven by ILoadingProcessInterface — this subsystem
 * simply returns true from ShouldShowLoadingScreen() until the Idle phase is
 * reached. No Lyra Experience or online system is required.
 *
 * Blueprint usage (from a menu button's OnClicked):
 *   GetGameInstance → GetSubsystem(MYSTLevelLoadingSubsystem)
 *     → bind OnLoadingComplete (optional)
 *     → call RequestLevelTravel("/Game/Maps/L_MyLevel", true)
 */
UCLASS(meta = (DisplayName = "MYST Level Loading Subsystem"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API UMYSTLevelLoadingSubsystem
	: public UGameInstanceSubsystem
	, public FTickableGameObject
	, public ILoadingProcessInterface
{
	GENERATED_BODY()

public:

	// -----------------------------------------------------------------------
	// USubsystem interface
	// -----------------------------------------------------------------------

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Provides a world context for Blueprint timer / audio nodes. */
	virtual UWorld* GetWorld() const override;

	// -----------------------------------------------------------------------
	// FTickableGameObject interface
	// -----------------------------------------------------------------------

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }
	virtual bool IsTickable() const override { return CurrentPhase != EMYSTLevelLoadPhase::Idle; }

	// -----------------------------------------------------------------------
	// ILoadingProcessInterface
	// -----------------------------------------------------------------------

	/**
	 * Returns true (keeps loading screen visible) whenever we are not in the
	 * Idle phase.
	 */
	virtual bool ShouldShowLoadingScreen(FString& OutReason) const override;

	// -----------------------------------------------------------------------
	// Public API — callable from Blueprint or C++
	// -----------------------------------------------------------------------

	/**
	 * Primary entry point. Shows the loading screen immediately, then travels
	 * to MapPath. Gates the loading screen until streaming and PSO compilation
	 * are both complete.
	 *
	 * @param MapPath   Full asset path or short map name (e.g. "/Game/Maps/L_Playground").
	 * @param bAbsolute If true, performs an absolute map change (destroys current world).
	 *                  If false, performs a relative travel (used for seamless travel).
	 */
	UFUNCTION(BlueprintCallable, Category = "MYST|Level Loading",
		meta = (DisplayName = "Request Level Travel"))
	void RequestLevelTravel(const FString& MapPath, bool bAbsolute = true);

	/** Current phase in the loading pipeline. */
	UFUNCTION(BlueprintPure, Category = "MYST|Level Loading")
	EMYSTLevelLoadPhase GetCurrentPhase() const { return CurrentPhase; }

	/**
	 * Normalized (0..1) shader precompilation progress.
	 * Returns 1.0 when no PSO work remains or PSO caching is not active.
	 */
	UFUNCTION(BlueprintPure, Category = "MYST|Level Loading")
	float GetShaderPrecompileProgress() const;

	/**
	 * Maximum seconds to wait in WaitingForStreaming or WaitingForPSO before
	 * forcibly advancing. Set to 0 to disable. Default: 30 s.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MYST|Level Loading",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float PhaseTimeoutSeconds = 30.f;

	/**
	 * Minimum seconds to hold the loading screen after all shader/streaming
	 * gates have cleared. Useful to let texture streaming finish visually.
	 * Default: 0.25 s.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MYST|Level Loading",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MinHoldAfterCompleteSeconds = 0.25f;

	// -----------------------------------------------------------------------
	// Blueprint-assignable delegates
	// -----------------------------------------------------------------------

	/** Fired right before OpenLevel is called — loading screen appears here. */
	UPROPERTY(BlueprintAssignable, Category = "MYST|Level Loading|Events")
	FMYSTLoadingSimpleDelegate OnLoadingStarted;

	/** Fired when all streaming levels / WP cells are visible. */
	UPROPERTY(BlueprintAssignable, Category = "MYST|Level Loading|Events")
	FMYSTLoadingSimpleDelegate OnStreamingComplete;

	/** Fired when PSO precompilation has drained (or was never active). */
	UPROPERTY(BlueprintAssignable, Category = "MYST|Level Loading|Events")
	FMYSTLoadingSimpleDelegate OnShadersComplete;

	/**
	 * Fired every tick while in the WaitingForPSO phase.
	 * Bind this to a progress bar widget.
	 */
	UPROPERTY(BlueprintAssignable, Category = "MYST|Level Loading|Events")
	FMYSTShaderProgressDelegate OnShaderProgressUpdated;

	/** Fired when the loading screen is about to hide and gameplay can begin. */
	UPROPERTY(BlueprintAssignable, Category = "MYST|Level Loading|Events")
	FMYSTLoadingSimpleDelegate OnLoadingComplete;

private:

	// -----------------------------------------------------------------------
	// Internal helpers
	// -----------------------------------------------------------------------

	/** Bound to FCoreUObjectDelegates::PostLoadMapWithWorld */
	void HandlePostLoadMap(UWorld* LoadedWorld);

	/** Advance from WaitingForStreaming → WaitingForPSO */
	void AdvanceToWaitingForPSO();

	/** Advance from WaitingForPSO → Idle (done) */
	void AdvanceToIdle();

	/** True when all requested streaming levels in World are loaded+visible. */
	bool AreAllStreamingLevelsReady(UWorld* World) const;

	/** Register / unregister this subsystem with ULoadingScreenManager. */
	void RegisterWithLoadingManager();
	void UnregisterFromLoadingManager();

	// -----------------------------------------------------------------------
	// State
	// -----------------------------------------------------------------------

	EMYSTLevelLoadPhase CurrentPhase = EMYSTLevelLoadPhase::Idle;

	/** Wall-clock time when the current phase started (for timeout). */
	double PhaseStartTime = 0.0;

	/** Wall-clock time when all gates first cleared (for MinHold). */
	double AllGatesClearedTime = 0.0;

	/** Total PSO precompiles at the start of the WaitingForPSO phase (for %). */
	int32 TotalPSOsAtStart = 0;

	/** Set after PostLoadMapWithWorld fires; used to avoid world pointer stale. */
	TWeakObjectPtr<UWorld> LoadedWorldRef;

	/** Handle for the PostLoadMapWithWorld delegate binding. */
	FDelegateHandle PostLoadMapDelegateHandle;

	/**
	 * Deferred travel — stored so the short timer callback can execute it.
	 *
	 * OpenLevel is intentionally delayed by ~0.15 s (≈9 frames @ 60fps) after
	 * RegisterWithLoadingManager() so that ULoadingScreenManager has enough time
	 * to complete its full show-screen pipeline:
	 *   Frame N   – ShouldShowLoadingScreen() returns true for the first time.
	 *   Frame N+1 – Manager.Tick() calls ShowLoadingScreen() and adds the Slate widget.
	 *   Frame N+2 – Slate renders the widget (first visible frame).
	 * Calling OpenLevel any sooner results in the game thread freezing before the
	 * loading screen is visually on-screen, causing the 2-3 s black screen artefact.
	 */
	void ExecutePendingLevelTravel();
	FString       PendingMapPath;
	bool          bPendingAbsolute = true;
	FTimerHandle  PendingTravelTimerHandle;
};

