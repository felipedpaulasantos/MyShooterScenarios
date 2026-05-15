// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "Settings/LyraSettingsShared.h" // EMYSTAimAssistPreset

#include "MYSTAimAssistPresetsDataAsset.generated.h"

/**
 * Per-preset tunable values applied to UAimAssistInputModifier at runtime.
 * Reticle sizes and target range are left unchanged across presets;
 * only strength and response rates change to maintain consistent target acquisition.
 *
 * Default values match the "Moderate" preset (recommended for most players).
 */
USTRUCT(BlueprintType)
struct FMYSTAimAssistPresetData
{
	GENERATED_BODY()

	/** Localized name shown in the settings UI option list. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display")
	FText DisplayName;

	/** Short description shown in the settings UI tooltip. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display")
	FText Description;

	// ── Pull (magnetic rotation toward enemy) ─────────────────────────────────

	/** How strongly the camera is pulled toward a target inside the inner reticle (hip-fire). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pull", meta=(ClampMin="0.0", ClampMax="1.0"))
	float PullInnerStrengthHip = 0.20f;

	/** How strongly the camera is pulled toward a target inside the outer reticle (hip-fire). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pull", meta=(ClampMin="0.0", ClampMax="1.0"))
	float PullOuterStrengthHip = 0.10f;

	/** How strongly the camera is pulled toward a target inside the inner reticle (ADS). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pull", meta=(ClampMin="0.0", ClampMax="1.0"))
	float PullInnerStrengthAds = 0.30f;

	/** How strongly the camera is pulled toward a target inside the outer reticle (ADS). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pull", meta=(ClampMin="0.0", ClampMax="1.0"))
	float PullOuterStrengthAds = 0.18f;

	/**
	 * Exponential rate at which pull strength ramps UP when acquiring a target.
	 * Lower values = smoother engagement (less snap). Replaces the broken default of 60.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pull", meta=(ClampMin="0.0"))
	float PullLerpInRate = 8.0f;

	/** Exponential rate at which pull strength ramps DOWN when losing a target. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pull", meta=(ClampMin="0.0"))
	float PullLerpOutRate = 4.0f;

	/** Maximum rotation rate ceiling for pull (degrees/sec). 0 = disabled. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pull", meta=(ClampMin="0.0"))
	float PullMaxRotationRate = 45.0f;

	// ── Slow (friction / sticky feel on hover) ────────────────────────────────

	/** Slow applied when target is inside the inner reticle (hip-fire). Keep at 0 to avoid a "stop" feel. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Slow", meta=(ClampMin="0.0", ClampMax="1.0"))
	float SlowInnerStrengthHip = 0.0f;

	/** Slow applied when target is inside the outer reticle (hip-fire). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Slow", meta=(ClampMin="0.0", ClampMax="1.0"))
	float SlowOuterStrengthHip = 0.04f;

	/** Slow applied when target is inside the inner reticle (ADS). Keep at 0 to avoid a "stop" feel. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Slow", meta=(ClampMin="0.0", ClampMax="1.0"))
	float SlowInnerStrengthAds = 0.0f;

	/** Slow applied when target is inside the outer reticle (ADS). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Slow", meta=(ClampMin="0.0", ClampMax="1.0"))
	float SlowOuterStrengthAds = 0.08f;

	/** Exponential rate at which slow ramps UP when acquiring a target. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Slow", meta=(ClampMin="0.0"))
	float SlowLerpInRate = 8.0f;

	/** Exponential rate at which slow ramps DOWN when losing a target. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Slow", meta=(ClampMin="0.0"))
	float SlowLerpOutRate = 4.0f;

	/** Minimum rotation rate floor during slow effect. 0 = disabled. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Slow", meta=(ClampMin="0.0"))
	float SlowMinRotationRate = 0.0f;
};

/**
 * UMYSTAimAssistPresetsDataAsset
 *
 * Stores the four aim-assist preset configurations (Disabled / Low / Moderate / Strong).
 * Create one instance at /MyShooterFeaturePlugin/Settings/DA_AimAssistPresets and
 * assign it to UMYSTAimAssistSettingsSubsystem::PresetsDataAsset in the subsystem BP or CDO.
 *
 * The UI widget reads DisplayName / Description from here.
 * UMYSTAimAssistSettingsSubsystem reads the full struct and pushes values to the live modifier.
 *
 * Array must have exactly 4 entries in EMYSTAimAssistPreset order:
 *   [0] Disabled, [1] Low, [2] Moderate, [3] Strong
 */
UCLASS(BlueprintType)
class MYSHOOTERFEATUREPLUGINRUNTIME_API UMYSTAimAssistPresetsDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UMYSTAimAssistPresetsDataAsset();

	/**
	 * Preset definitions, one per EMYSTAimAssistPreset value.
	 * Index 0 = Disabled, 1 = Low, 2 = Moderate, 3 = Strong.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AimAssist")
	TArray<FMYSTAimAssistPresetData> Presets;

	/** Returns the preset data for the given enum value, or a fallback if out of range. */
	const FMYSTAimAssistPresetData& GetPresetData(EMYSTAimAssistPreset Preset) const;
};

