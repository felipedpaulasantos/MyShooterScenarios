// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraAssetManager.h"
#include "LyraLogChannels.h"
#include "LyraGameplayTags.h"
#include "LyraGameData.h"
#include "AbilitySystemGlobals.h"
#include "Character/LyraPawnData.h"
#include "Misc/App.h"
#include "Stats/StatsMisc.h"
#include "Engine/Engine.h"
#include "AbilitySystem/LyraGameplayCueManager.h"
#include "Misc/ScopedSlowTask.h"
#include "System/LyraAssetManagerStartupJob.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraAssetManager)

const FName FLyraBundles::Equipped("Equipped");

//////////////////////////////////////////////////////////////////////

static FAutoConsoleCommand CVarDumpLoadedAssets(
	TEXT("Lyra.DumpLoadedAssets"),
	TEXT("Shows all assets that were loaded via the asset manager and are currently in memory."),
	FConsoleCommandDelegate::CreateStatic(ULyraAssetManager::DumpLoadedAssets)
);

//////////////////////////////////////////////////////////////////////

#define STARTUP_JOB_WEIGHTED(JobFunc, JobWeight) StartupJobs.Add(FLyraAssetManagerStartupJob(#JobFunc, [this](const FLyraAssetManagerStartupJob& StartupJob, TSharedPtr<FStreamableHandle>& LoadHandle){JobFunc;}, JobWeight))
#define STARTUP_JOB(JobFunc) STARTUP_JOB_WEIGHTED(JobFunc, 1.f)

//////////////////////////////////////////////////////////////////////

ULyraAssetManager::ULyraAssetManager()
{
	DefaultPawnData = nullptr;
}

ULyraAssetManager& ULyraAssetManager::Get()
{
	check(GEngine);

	if (ULyraAssetManager* Singleton = Cast<ULyraAssetManager>(GEngine->AssetManager))
	{
		return *Singleton;
	}

	UE_LOG(LogLyra, Fatal, TEXT("Invalid AssetManagerClassName in DefaultEngine.ini.  It must be set to LyraAssetManager!"));

	// Fatal error above prevents this from being called.
	return *NewObject<ULyraAssetManager>();
}

UObject* ULyraAssetManager::SynchronousLoadAsset(const FSoftObjectPath& AssetPath)
{
	if (AssetPath.IsValid())
	{
		TUniquePtr<FScopeLogTime> LogTimePtr;

		if (ShouldLogAssetLoads())
		{
			LogTimePtr = MakeUnique<FScopeLogTime>(*FString::Printf(TEXT("Synchronously loaded asset [%s]"), *AssetPath.ToString()), nullptr, FScopeLogTime::ScopeLog_Seconds);
		}

		if (UAssetManager::Get().IsInitialized())
		{
			return UAssetManager::GetStreamableManager().LoadSynchronous(AssetPath, false);
		}

		// Use LoadObject if asset manager isn't ready yet.
		return AssetPath.TryLoad();
	}

	return nullptr;
}

bool ULyraAssetManager::ShouldLogAssetLoads()
{
	static bool bLogAssetLoads = FParse::Param(FCommandLine::Get(), TEXT("LogAssetLoads"));
	return bLogAssetLoads;
}

void ULyraAssetManager::AddLoadedAsset(const UObject* Asset)
{
	if (ensureAlways(Asset))
	{
		FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
		LoadedAssets.Add(Asset);
	}
}

void ULyraAssetManager::DumpLoadedAssets()
{
	UE_LOG(LogLyra, Log, TEXT("========== Start Dumping Loaded Assets =========="));

	for (const UObject* LoadedAsset : Get().LoadedAssets)
	{
		UE_LOG(LogLyra, Log, TEXT("  %s"), *GetNameSafe(LoadedAsset));
	}

	UE_LOG(LogLyra, Log, TEXT("... %d assets in loaded pool"), Get().LoadedAssets.Num());
	UE_LOG(LogLyra, Log, TEXT("========== Finish Dumping Loaded Assets =========="));
}

void ULyraAssetManager::StartInitialLoading()
{
	SCOPED_BOOT_TIMING("ULyraAssetManager::StartInitialLoading");

	// This does all of the scanning, need to do this now even if loads are deferred
	Super::StartInitialLoading();

	STARTUP_JOB(InitializeAbilitySystem());
	STARTUP_JOB(InitializeGameplayCueManager());

	// Only try to load GameData if the path is configured
	if (!LyraGameDataPath.IsNull())
	{
		// Load base game data asset
		STARTUP_JOB_WEIGHTED(GetGameData(), 25.f);
	}
	else
	{
		UE_LOG(LogLyra, Warning, TEXT("LyraGameDataPath is not configured in DefaultGame.ini. Skipping GameData load."));
	}

	// Run all the queued up startup jobs
	DoAllStartupJobs();
}

void ULyraAssetManager::InitializeAbilitySystem()
{
	SCOPED_BOOT_TIMING("ULyraAssetManager::InitializeAbilitySystem");

	UAbilitySystemGlobals::Get().InitGlobalData();
}

void ULyraAssetManager::InitializeGameplayCueManager()
{
	SCOPED_BOOT_TIMING("ULyraAssetManager::InitializeGameplayCueManager");

	ULyraGameplayCueManager* GCM = ULyraGameplayCueManager::Get();
	check(GCM);
	GCM->LoadAlwaysLoadedCues();
}


const ULyraGameData& ULyraAssetManager::GetGameData()
{
	// Check if we have a valid path configured
	if (LyraGameDataPath.IsNull())
	{
		// Create a warning but try to use a default fallback
		static bool bWarningShown = false;
		if (!bWarningShown)
		{
			UE_LOG(LogLyra, Warning, TEXT("LyraGameDataPath is not configured. Game systems may not function correctly. Please create DefaultGameData asset and configure it in DefaultGame.ini"));
			bWarningShown = true;
		}
		
		// Try to create a minimal fallback GameData to prevent crashes
		static ULyraGameData* FallbackGameData = nullptr;
		if (!FallbackGameData)
		{
			FallbackGameData = NewObject<ULyraGameData>();
			FallbackGameData->AddToRoot(); // Prevent garbage collection
			UE_LOG(LogLyra, Warning, TEXT("Using fallback GameData object. Core gameplay effects will not be available."));
		}
		return *FallbackGameData;
	}
	
	return GetOrLoadTypedGameData<ULyraGameData>(LyraGameDataPath);
}

const ULyraPawnData* ULyraAssetManager::GetDefaultPawnData() const
{
	return GetAsset(DefaultPawnData);
}

UPrimaryDataAsset* ULyraAssetManager::LoadGameDataOfClass(TSubclassOf<UPrimaryDataAsset> DataClass, const TSoftObjectPtr<UPrimaryDataAsset>& DataClassPath, FPrimaryAssetType PrimaryAssetType)
{
	UPrimaryDataAsset* Asset = nullptr;

	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Loading GameData Object"), STAT_GameData, STATGROUP_LoadTime);
	if (!DataClassPath.IsNull())
	{
#if WITH_EDITOR
		FScopedSlowTask SlowTask(0, FText::Format(NSLOCTEXT("LyraEditor", "BeginLoadingGameDataTask", "Loading GameData {0}"), FText::FromName(DataClass->GetFName())));
		const bool bShowCancelButton = false;
		const bool bAllowInPIE = true;
		SlowTask.MakeDialog(bShowCancelButton, bAllowInPIE);
#endif
		UE_LOG(LogLyra, Log, TEXT("Loading GameData: %s ..."), *DataClassPath.ToString());
		SCOPE_LOG_TIME_IN_SECONDS(TEXT("    ... GameData loaded!"), nullptr);

		// This can be called recursively in the editor because it is called on demand from PostLoad so force a sync load for primary asset and async load the rest in that case
		if (GIsEditor)
		{
			Asset = DataClassPath.LoadSynchronous();
			LoadPrimaryAssetsWithType(PrimaryAssetType);
		}
		else
		{
			TSharedPtr<FStreamableHandle> Handle = LoadPrimaryAssetsWithType(PrimaryAssetType);
			if (Handle.IsValid())
			{
				Handle->WaitUntilComplete(0.0f, false);

				// This should always work
				Asset = Cast<UPrimaryDataAsset>(Handle->GetLoadedAsset());
			}
		}
	}

	if (Asset)
	{
		GameDataMap.Add(DataClass, Asset);
	}
	else
	{
		// Changed from Fatal to Error to prevent crash - allow game to continue with warnings
		UE_LOG(LogLyra, Error, TEXT("Failed to load GameData asset at %s. Type %s. Game may not function correctly without this data. Please create this asset or update the path in DefaultGame.ini"), *DataClassPath.ToString(), *PrimaryAssetType.ToString());
	}

	return Asset;
}


void ULyraAssetManager::DoAllStartupJobs()
{
	SCOPED_BOOT_TIMING("ULyraAssetManager::DoAllStartupJobs");
	const double AllStartupJobsStartTime = FPlatformTime::Seconds();

	if (IsRunningDedicatedServer())
	{
		// No need for periodic progress updates, just run the jobs
		for (const FLyraAssetManagerStartupJob& StartupJob : StartupJobs)
		{
			StartupJob.DoJob();
		}
	}
	else
	{
		if (StartupJobs.Num() > 0)
		{
			float TotalJobValue = 0.0f;
			for (const FLyraAssetManagerStartupJob& StartupJob : StartupJobs)
			{
				TotalJobValue += StartupJob.JobWeight;
			}

			float AccumulatedJobValue = 0.0f;
			for (FLyraAssetManagerStartupJob& StartupJob : StartupJobs)
			{
				const float JobValue = StartupJob.JobWeight;
				StartupJob.SubstepProgressDelegate.BindLambda([This = this, AccumulatedJobValue, JobValue, TotalJobValue](float NewProgress)
					{
						const float SubstepAdjustment = FMath::Clamp(NewProgress, 0.0f, 1.0f) * JobValue;
						const float OverallPercentWithSubstep = (AccumulatedJobValue + SubstepAdjustment) / TotalJobValue;

						This->UpdateInitialGameContentLoadPercent(OverallPercentWithSubstep);
					});

				StartupJob.DoJob();

				StartupJob.SubstepProgressDelegate.Unbind();

				AccumulatedJobValue += JobValue;

				UpdateInitialGameContentLoadPercent(AccumulatedJobValue / TotalJobValue);
			}
		}
		else
		{
			UpdateInitialGameContentLoadPercent(1.0f);
		}
	}

	StartupJobs.Empty();

	UE_LOG(LogLyra, Display, TEXT("All startup jobs took %.2f seconds to complete"), FPlatformTime::Seconds() - AllStartupJobsStartTime);
}

void ULyraAssetManager::UpdateInitialGameContentLoadPercent(float GameContentPercent)
{
	// Could route this to the early startup loading screen
}

#if WITH_EDITOR
void ULyraAssetManager::PreBeginPIE(bool bStartSimulate)
{
	Super::PreBeginPIE(bStartSimulate);

	{
		FScopedSlowTask SlowTask(0, NSLOCTEXT("LyraEditor", "BeginLoadingPIEData", "Loading PIE Data"));
		const bool bShowCancelButton = false;
		const bool bAllowInPIE = true;
		SlowTask.MakeDialog(bShowCancelButton, bAllowInPIE);

		const ULyraGameData& LocalGameDataCommon = GetGameData();

		// Intentionally after GetGameData to avoid counting GameData time in this timer
		SCOPE_LOG_TIME_IN_SECONDS(TEXT("PreBeginPIE asset preloading complete"), nullptr);

		// You could add preloading of anything else needed for the experience we'll be using here
		// (e.g., by grabbing the default experience from the world settings + the experience override in developer settings)
	}
}
#endif
