// Copyright MyShooterScenarios. All Rights Reserved.

#include "LevelLoading/MYSTLevelLoadingSubsystem.h"
#include "LoadingScreenManager.h"
#include "Engine/World.h"
#include "Engine/LevelStreaming.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CoreDelegates.h"

// UE5 PSO / shader precompile polling
// FShaderPipelineCache lives in RenderCore
#include "ShaderPipelineCache.h"

// IMoviePlayer — keeps the loading screen ticking during the synchronous LoadMap block
#include "MoviePlayer.h"
#include "Widgets/SNullWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MYSTLevelLoadingSubsystem)

// ---------------------------------------------------------------------------
// USubsystem
// ---------------------------------------------------------------------------

void UMYSTLevelLoadingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Ensure ULoadingScreenManager is initialized before us so we can register.
	Collection.InitializeDependency<ULoadingScreenManager>();

	// Listen for the moment the new map world is fully constructed. The
	// ULoadingScreenManager handles the PreLoadMap→PostLoadMap window itself;
	// we take over from PostLoadMap to gate streaming + PSO phases.
	PostLoadMapDelegateHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UMYSTLevelLoadingSubsystem::HandlePostLoadMap);
}

void UMYSTLevelLoadingSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapDelegateHandle);
	UnregisterFromLoadingManager();

	Super::Deinitialize();
}

UWorld* UMYSTLevelLoadingSubsystem::GetWorld() const
{
	if (const UGameInstance* GI = GetGameInstance())
	{
		return GI->GetWorld();
	}
	return nullptr;
}

// ---------------------------------------------------------------------------
// FTickableGameObject
// ---------------------------------------------------------------------------

TStatId UMYSTLevelLoadingSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UMYSTLevelLoadingSubsystem, STATGROUP_Tickables);
}

void UMYSTLevelLoadingSubsystem::Tick(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();

	// ---- WaitingForMap ------------------------------------------------
	// The ULoadingScreenManager already handles the PreLoadMap→PostLoadMap
	// window. We just sit here until HandlePostLoadMap() advances us.
	if (CurrentPhase == EMYSTLevelLoadPhase::WaitingForMap)
	{
		// Timeout safety — if PostLoadMap never fires (e.g. travel failed)
		if (PhaseTimeoutSeconds > 0.f && (Now - PhaseStartTime) > PhaseTimeoutSeconds)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("MYSTLevelLoadingSubsystem: WaitingForMap timed out after %.1f s — advancing to Idle."),
				PhaseTimeoutSeconds);
			AdvanceToIdle();
		}
		return;
	}

	// ---- WaitingForStreaming -------------------------------------------
	if (CurrentPhase == EMYSTLevelLoadPhase::WaitingForStreaming)
	{
		UWorld* World = LoadedWorldRef.Get();

		// Timeout safety
		if (PhaseTimeoutSeconds > 0.f && (Now - PhaseStartTime) > PhaseTimeoutSeconds)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("MYSTLevelLoadingSubsystem: WaitingForStreaming timed out after %.1f s — advancing to PSO phase."),
				PhaseTimeoutSeconds);
			AdvanceToWaitingForPSO();
			return;
		}

		if (World && AreAllStreamingLevelsReady(World))
		{
			AdvanceToWaitingForPSO();
		}
		return;
	}

	// ---- WaitingForPSO ------------------------------------------------
	if (CurrentPhase == EMYSTLevelLoadPhase::WaitingForPSO)
	{
		const int32 Remaining = static_cast<int32>(FShaderPipelineCache::NumPrecompilesRemaining());
		const int32 Total     = FMath::Max(TotalPSOsAtStart, 1); // avoid div/0
		const float Progress  = 1.f - FMath::Clamp((float)Remaining / (float)Total, 0.f, 1.f);

		OnShaderProgressUpdated.Broadcast(Progress, Remaining);

		// Timeout safety
		if (PhaseTimeoutSeconds > 0.f && (Now - PhaseStartTime) > PhaseTimeoutSeconds)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("MYSTLevelLoadingSubsystem: WaitingForPSO timed out after %.1f s — forcing completion."),
				PhaseTimeoutSeconds);
			AdvanceToIdle();
			return;
		}

		if (Remaining == 0)
		{
			// Gate cleared — start MinHold timer on first clear tick
			if (AllGatesClearedTime <= 0.0)
			{
				AllGatesClearedTime = Now;
			}

			if ((Now - AllGatesClearedTime) >= (double)MinHoldAfterCompleteSeconds)
			{
				AdvanceToIdle();
			}
		}
		else
		{
			// PSO work resumed (unlikely but reset hold timer to be safe)
			AllGatesClearedTime = 0.0;
		}

		return;
	}
}

// ---------------------------------------------------------------------------
// ILoadingProcessInterface
// ---------------------------------------------------------------------------

bool UMYSTLevelLoadingSubsystem::ShouldShowLoadingScreen(FString& OutReason) const
{
	switch (CurrentPhase)
	{
	case EMYSTLevelLoadPhase::WaitingForMap:
		OutReason = TEXT("MYST: Waiting for map to load");
		return true;
	case EMYSTLevelLoadPhase::WaitingForStreaming:
		OutReason = TEXT("MYST: Waiting for streaming levels");
		return true;
	case EMYSTLevelLoadPhase::WaitingForPSO:
		OutReason = TEXT("MYST: Waiting for shader precompilation");
		return true;
	default:
		return false;
	}
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void UMYSTLevelLoadingSubsystem::RequestLevelTravel(const FString& MapPath, bool bAbsolute)
{
	if (CurrentPhase != EMYSTLevelLoadPhase::Idle)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("MYSTLevelLoadingSubsystem: RequestLevelTravel called while already loading (%s). Ignoring."),
			*MapPath);
		return;
	}

	UWorld* World = GetWorld();
	if (!ensureMsgf(World, TEXT("MYSTLevelLoadingSubsystem: No world available for RequestLevelTravel.")))
	{
		return;
	}

	// Transition to WaitingForMap and register with the loading screen manager
	// BEFORE calling OpenLevel so the screen is up from the very first frame.
	CurrentPhase    = EMYSTLevelLoadPhase::WaitingForMap;
	PhaseStartTime  = FPlatformTime::Seconds();
	LoadedWorldRef  = nullptr;
	AllGatesClearedTime = 0.0;
	TotalPSOsAtStart    = 0;

	RegisterWithLoadingManager();
	OnLoadingStarted.Broadcast();

	// Defer OpenLevel by ~0.15 s so ULoadingScreenManager has enough frames to
	// fully show the loading screen before the game thread is blocked by the
	// synchronous level load (FlushAsyncLoading etc.).
	//
	// One tick is NOT sufficient: ULoadingScreenManager requires at least 3 frames
	// to (1) evaluate ShouldShowLoadingScreen(), (2) call ShowLoadingScreen() and
	// add the Slate widget, and (3) have Slate actually render it. Calling
	// OpenLevel any sooner causes a 2-3 s black screen before the loading screen
	// appears. 0.15 s (~9 frames @ 60 fps) gives a comfortable safety margin.
	PendingMapPath   = MapPath;
	bPendingAbsolute = bAbsolute;
	GetGameInstance()->GetTimerManager().SetTimer(
		PendingTravelTimerHandle,
		this, &UMYSTLevelLoadingSubsystem::ExecutePendingLevelTravel,
		0.15f,
		/*bLoop=*/false);
}

void UMYSTLevelLoadingSubsystem::ExecutePendingLevelTravel()
{
	UWorld* World = GetWorld();
	if (!World || CurrentPhase == EMYSTLevelLoadPhase::Idle)
	{
		// Cancelled or world gone — bail out cleanly
		AdvanceToIdle();
		return;
	}

	// -----------------------------------------------------------------------
	// Keep the loading screen ANIMATED during the synchronous LoadMap phase.
	//
	// Problem: OpenLevel → LoadMap → FlushAsyncLoading() locks the game thread.
	// Because Slate ticks on the game thread, the loading screen widget freezes
	// for the entire duration of the synchronous load (typically 3–10 seconds).
	//
	// Solution: Register a FLoadingScreenAttributes with bAllowEngineTick=true
	// in the Movie Player BEFORE calling OpenLevel. The Movie Player listens to
	// FCoreUObjectDelegates::PreLoadMap and activates its own loading thread,
	// which calls back into engine tick (including Slate rendering) between each
	// package load. This keeps spinner/progress animations running.
	//
	// We supply SNullWidget as the movie player widget (transparent, zero-size)
	// so the existing CommonLoadingScreen Slate overlay shows through. The Movie
	// Player only needs a valid widget pointer to consider itself "prepared".
	// -----------------------------------------------------------------------
	if (GetMoviePlayer() && GetMoviePlayer()->IsInitialized())
	{
		FLoadingScreenAttributes TickableScreen;
		TickableScreen.bAutoCompleteWhenLoadingCompletes = true;
		TickableScreen.bWaitForManualStop               = false;
		TickableScreen.bAllowEngineTick                 = true;
		TickableScreen.MinimumLoadingScreenDisplayTime  = 0.f;
		// Transparent placeholder — activates the movie player system without
		// visually overlaying the CommonLoadingScreen widget.
		TickableScreen.WidgetLoadingScreen = SNullWidget::NullWidget;

		GetMoviePlayer()->SetupLoadingScreen(TickableScreen);
	}

	UGameplayStatics::OpenLevel(World, FName(*PendingMapPath), bPendingAbsolute);
}

float UMYSTLevelLoadingSubsystem::GetShaderPrecompileProgress() const{
	if (CurrentPhase != EMYSTLevelLoadPhase::WaitingForPSO)
	{
		return CurrentPhase == EMYSTLevelLoadPhase::Idle ? 1.f : 0.f;
	}

	const int32 Remaining = static_cast<int32>(FShaderPipelineCache::NumPrecompilesRemaining());
	const int32 Total     = FMath::Max(TotalPSOsAtStart, 1);
	return 1.f - FMath::Clamp((float)Remaining / (float)Total, 0.f, 1.f);
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

void UMYSTLevelLoadingSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	// Only care about this event when we initiated a travel
	if (CurrentPhase != EMYSTLevelLoadPhase::WaitingForMap)
	{
		return;
	}

	// If we get called with a null world somehow, time out gracefully
	if (!LoadedWorld)
	{
		UE_LOG(LogTemp, Warning, TEXT("MYSTLevelLoadingSubsystem: PostLoadMap fired with null world."));
		AdvanceToIdle();
		return;
	}

	LoadedWorldRef = LoadedWorld;
	CurrentPhase   = EMYSTLevelLoadPhase::WaitingForStreaming;
	PhaseStartTime = FPlatformTime::Seconds();

	UE_LOG(LogTemp, Log,
		TEXT("MYSTLevelLoadingSubsystem: Map '%s' loaded — entering WaitingForStreaming phase."),
		*LoadedWorld->GetName());
}

void UMYSTLevelLoadingSubsystem::AdvanceToWaitingForPSO()
{
	OnStreamingComplete.Broadcast();

	CurrentPhase     = EMYSTLevelLoadPhase::WaitingForPSO;
	PhaseStartTime   = FPlatformTime::Seconds();
	AllGatesClearedTime = 0.0;
	TotalPSOsAtStart = static_cast<int32>(FMath::Max(FShaderPipelineCache::NumPrecompilesRemaining(), 1u));

	UE_LOG(LogTemp, Log,
		TEXT("MYSTLevelLoadingSubsystem: Streaming complete — entering WaitingForPSO phase (%d shaders remaining)."),
		TotalPSOsAtStart);
}

void UMYSTLevelLoadingSubsystem::AdvanceToIdle()
{
	const bool bWasPSO = (CurrentPhase == EMYSTLevelLoadPhase::WaitingForPSO);

	// Broadcast intermediate events if we skipped phases (e.g. timeout path)
	if (CurrentPhase == EMYSTLevelLoadPhase::WaitingForStreaming)
	{
		OnStreamingComplete.Broadcast();
	}
	if (bWasPSO || CurrentPhase == EMYSTLevelLoadPhase::WaitingForStreaming)
	{
		OnShadersComplete.Broadcast();
	}

	CurrentPhase = EMYSTLevelLoadPhase::Idle;
	AllGatesClearedTime = 0.0;

	UnregisterFromLoadingManager();
	OnLoadingComplete.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("MYSTLevelLoadingSubsystem: Loading complete — loading screen released."));
}

bool UMYSTLevelLoadingSubsystem::AreAllStreamingLevelsReady(UWorld* World) const
{
	if (!World)
	{
		return true;
	}

	// Check global visibility request queue — covers both classic streaming and
	// World Partition cell visibility requests.
	if (World->IsVisibilityRequestPending())
	{
		return false;
	}

	// Classic streaming levels — wait for all that should be loaded+visible.
	// World Partition also registers its cells as ULevelStreaming objects, so
	// this single loop covers both systems safely.
	for (ULevelStreaming* Level : World->GetStreamingLevels())
	{
		if (!Level)
		{
			continue;
		}

		if (Level->ShouldBeLoaded() && !Level->IsLevelLoaded())
		{
			return false;
		}

		if (Level->ShouldBeVisible() && !Level->IsLevelVisible())
		{
			return false;
		}
	}

	return true;
}

void UMYSTLevelLoadingSubsystem::RegisterWithLoadingManager()
{
	if (ULoadingScreenManager* Manager = GetGameInstance()->GetSubsystem<ULoadingScreenManager>())
	{
		Manager->RegisterLoadingProcessor(TScriptInterface<ILoadingProcessInterface>(this));
		UE_LOG(LogTemp, Verbose, TEXT("MYSTLevelLoadingSubsystem: Registered with ULoadingScreenManager."));
	}
}

void UMYSTLevelLoadingSubsystem::UnregisterFromLoadingManager()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (ULoadingScreenManager* Manager = GI->GetSubsystem<ULoadingScreenManager>())
		{
			Manager->UnregisterLoadingProcessor(TScriptInterface<ILoadingProcessInterface>(this));
			UE_LOG(LogTemp, Verbose, TEXT("MYSTLevelLoadingSubsystem: Unregistered from ULoadingScreenManager."));
		}
	}
}



