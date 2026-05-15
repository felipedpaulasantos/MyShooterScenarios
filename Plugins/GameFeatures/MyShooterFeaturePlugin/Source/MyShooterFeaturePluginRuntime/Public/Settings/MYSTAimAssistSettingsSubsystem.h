// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "Settings/MYSTAimAssistPresetsDataAsset.h"

#include "MYSTAimAssistSettingsSubsystem.generated.h"

class ULyraSettingsShared;

/**
 * UMYSTAimAssistSettingsSubsystem
 *
 * Watches ULyraSettingsShared::AimAssistPreset and pushes the matching
 * FMYSTAimAssistPresetData values onto every live UAimAssistInputModifier
 * instance in the game.
 *
 * ── How to wire it up ───────────────────────────────────────────────────────
 * Apply is triggered automatically when the player changes the setting via the
 * settings menu (OnSettingChanged fires). However, because Enhanced Input rebuilds
 * its key mappings each time InitializePlayerInput is called, you must also call
 * ApplyCurrentPreset() after input is initialized on each pawn possession:
 *
 *   In your Hero Blueprint → override "On Initialize Player Input" →
 *   Get Game Instance → Get Subsystem (MYSTAimAssistSettingsSubsystem) →
 *   Call ApplyCurrentPreset.
 *
 * ── DataAsset setup ──────────────────────────────────────────────────────────
 * Create a UMYSTAimAssistPresetsDataAsset at:
 *   /MyShooterFeaturePlugin/Settings/DA_AimAssistPresets
 * and assign its soft reference to PresetsDataAsset below (or leave the
 * default path and the subsystem will load it automatically).
 * The DataAsset ships with sensible defaults in its constructor, so it works
 * without any manual editing.
 */
UCLASS()
class MYSHOOTERFEATUREPLUGINRUNTIME_API UMYSTAimAssistSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	//~ USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem interface

	/**
	 * Soft reference to the DataAsset that contains the four preset definitions.
	 * Defaults to /MyShooterFeaturePlugin/Settings/DA_AimAssistPresets.
	 * Assign a different asset here (or in a Blueprint subclass) to override.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="AimAssist", meta=(AssetBundles="Client"))
	TSoftObjectPtr<UMYSTAimAssistPresetsDataAsset> PresetsDataAsset;

	/**
	 * Read the current preset from LyraSettingsShared and apply its values to every
	 * UAimAssistInputModifier that exists in memory.
	 *
	 * Call this from your Hero Blueprint's "On Initialize Player Input" override
	 * to ensure fresh values are applied after each pawn possession.
	 */
	UFUNCTION(BlueprintCallable, Category="AimAssist")
	void ApplyCurrentPreset();

	/** Returns the DataAsset, loading it synchronously if necessary. May return nullptr. */
	UFUNCTION(BlueprintPure, Category="AimAssist")
	UMYSTAimAssistPresetsDataAsset* GetPresetsDataAsset() const;

private:

	void OnSettingsChanged(ULyraSettingsShared* SharedSettings);
	void ApplyPresetData(const FMYSTAimAssistPresetData& Data) const;

	FDelegateHandle SettingsChangedHandle;
};

