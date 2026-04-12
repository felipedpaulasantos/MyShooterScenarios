// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "MusicManagerSubsystem.generated.h"

/**
 * Thin Blueprintable wrapper over UGameInstanceSubsystem.
 * 
 * Subclass this in Blueprint (BP_MusicManagerSubsystem) to implement
 * all music management logic there. This C++ class exists solely because
 * UGameInstanceSubsystem is not Blueprintable by default.
 *
 * Lifecycle:
 *   Initialize  → calls K2_OnInitialize  (BeginPlay equivalent)
 *   Deinitialize → calls K2_OnDeinitialize (EndPlay equivalent)
 *
 * Usage:
 *   - Get from anywhere: GameInstance → Get Subsystem (BP_MusicManagerSubsystem)
 *   - Change context:    Subsystem → RequestMusicContext(Music.Context.Gameplay)
 */
UCLASS(Blueprintable, meta = (DisplayName = "Music Manager Subsystem"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API UMusicManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	// -----------------------------------------------------------------------
	// USubsystem interface
	// -----------------------------------------------------------------------

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Provides a world context so Blueprint nodes (timers, audio, etc.) work without a manual pin. */
	virtual UWorld* GetWorld() const override;

	// -----------------------------------------------------------------------
	// Public API — callable from any Blueprint or C++
	// -----------------------------------------------------------------------

	/**
	 * Request a music context transition (e.g. Music.Context.Gameplay).
	 * The Blueprint implementation handles cross-fading and state tracking.
	 * Pass Music.Context.None to gracefully fade to silence.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Music")
	void RequestMusicContext(FGameplayTag ContextTag);
	virtual void RequestMusicContext_Implementation(FGameplayTag ContextTag) {}

	/** Returns the currently active music context tag. */
	UFUNCTION(BlueprintPure, Category = "Music")
	FGameplayTag GetCurrentMusicContext() const { return CurrentContextTag; }

protected:

	// -----------------------------------------------------------------------
	// Blueprint lifecycle hooks  (implement these in BP_MusicManagerSubsystem)
	// -----------------------------------------------------------------------

	/** Called after the subsystem is initialized. Set up AudioComponent, register message listeners here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Music|Lifecycle", meta = (DisplayName = "On Initialize"))
	void K2_OnInitialize();

	/** Called before the subsystem is torn down. Release AudioComponent here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Music|Lifecycle", meta = (DisplayName = "On Deinitialize"))
	void K2_OnDeinitialize();

	// -----------------------------------------------------------------------
	// State — writable by the Blueprint subclass
	// -----------------------------------------------------------------------

	/** The currently active music context. Updated by the BP implementation inside RequestMusicContext. */
	UPROPERTY(BlueprintReadWrite, Category = "Music")
	FGameplayTag CurrentContextTag;
};

