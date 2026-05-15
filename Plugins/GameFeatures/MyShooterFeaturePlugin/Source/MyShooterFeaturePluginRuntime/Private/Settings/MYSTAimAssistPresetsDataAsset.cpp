// Copyright Epic Games, Inc. All Rights Reserved.

#include "Settings/MYSTAimAssistPresetsDataAsset.h"
// LyraSettingsShared.h (for EMYSTAimAssistPreset) is included transitively from the .h above.

#include UE_INLINE_GENERATED_CPP_BY_NAME(MYSTAimAssistPresetsDataAsset)

// ── Default preset table ──────────────────────────────────────────────────────
// These values match the recommendations derived from play-testing.
// Override them by editing the DA_AimAssistPresets DataAsset in the editor.

namespace MYSTAimAssist
{
	// Disabled — all effects zeroed, modifier still present so the system runs cleanly.
	static const FMYSTAimAssistPresetData Disabled = []
	{
		FMYSTAimAssistPresetData D;
		D.DisplayName              = NSLOCTEXT("MYST", "AimAssist_Disabled", "Disabled");
		D.Description              = NSLOCTEXT("MYST", "AimAssist_Disabled_Desc", "No aim assist. Full manual control.");
		D.PullInnerStrengthHip    = 0.f; D.PullOuterStrengthHip    = 0.f;
		D.PullInnerStrengthAds    = 0.f; D.PullOuterStrengthAds    = 0.f;
		D.PullLerpInRate           = 8.f; D.PullLerpOutRate          = 4.f;
		D.PullMaxRotationRate      = 0.f;
		D.SlowInnerStrengthHip    = 0.f; D.SlowOuterStrengthHip    = 0.f;
		D.SlowInnerStrengthAds    = 0.f; D.SlowOuterStrengthAds    = 0.f;
		D.SlowLerpInRate           = 8.f; D.SlowLerpOutRate          = 4.f;
		D.SlowMinRotationRate      = 0.f;
		return D;
	}();

	// Low — light guidance, barely perceptible, good for experienced players.
	static const FMYSTAimAssistPresetData Low = []
	{
		FMYSTAimAssistPresetData D;
		D.DisplayName              = NSLOCTEXT("MYST", "AimAssist_Low",      "Low");
		D.Description              = NSLOCTEXT("MYST", "AimAssist_Low_Desc", "Light guidance. Subtle pull with minimal slowdown.");
		D.PullInnerStrengthHip    = 0.10f; D.PullOuterStrengthHip    = 0.05f;
		D.PullInnerStrengthAds    = 0.15f; D.PullOuterStrengthAds    = 0.09f;
		D.PullLerpInRate           = 8.f;   D.PullLerpOutRate          = 4.f;
		D.PullMaxRotationRate      = 45.f;
		D.SlowInnerStrengthHip    = 0.f;    D.SlowOuterStrengthHip    = 0.02f;
		D.SlowInnerStrengthAds    = 0.f;    D.SlowOuterStrengthAds    = 0.04f;
		D.SlowLerpInRate           = 8.f;   D.SlowLerpOutRate          = 4.f;
		D.SlowMinRotationRate      = 0.f;
		return D;
	}();

	// Moderate — balanced and smooth; recommended for most players.
	// Pull strength from recommended tuning, lerp in fixed from 60→8.
	static const FMYSTAimAssistPresetData Moderate = []
	{
		FMYSTAimAssistPresetData D;
		D.DisplayName              = NSLOCTEXT("MYST", "AimAssist_Moderate",      "Moderate");
		D.Description              = NSLOCTEXT("MYST", "AimAssist_Moderate_Desc", "Balanced assist. Smooth engagement with gentle pull and slowdown.");
		D.PullInnerStrengthHip    = 0.20f; D.PullOuterStrengthHip    = 0.10f;
		D.PullInnerStrengthAds    = 0.30f; D.PullOuterStrengthAds    = 0.18f;
		D.PullLerpInRate           = 8.f;   D.PullLerpOutRate          = 4.f;
		D.PullMaxRotationRate      = 45.f;
		D.SlowInnerStrengthHip    = 0.f;    D.SlowOuterStrengthHip    = 0.04f;
		D.SlowInnerStrengthAds    = 0.f;    D.SlowOuterStrengthAds    = 0.08f;
		D.SlowLerpInRate           = 8.f;   D.SlowLerpOutRate          = 4.f;
		D.SlowMinRotationRate      = 0.f;
		return D;
	}();

	// Strong — original project values but with the fixed lerp-in rate.
	// Noticeably assistive without the camera-stop produced by the old lerp of 60.
	static const FMYSTAimAssistPresetData Strong = []
	{
		FMYSTAimAssistPresetData D;
		D.DisplayName              = NSLOCTEXT("MYST", "AimAssist_Strong",      "Strong");
		D.Description              = NSLOCTEXT("MYST", "AimAssist_Strong_Desc", "Aggressive assist. Strong pull and friction. Good for new players.");
		D.PullInnerStrengthHip    = 0.40f; D.PullOuterStrengthHip    = 0.20f;
		D.PullInnerStrengthAds    = 0.60f; D.PullOuterStrengthAds    = 0.40f;
		D.PullLerpInRate           = 8.f;   D.PullLerpOutRate          = 4.f;
		D.PullMaxRotationRate      = 45.f;
		D.SlowInnerStrengthHip    = 0.f;    D.SlowOuterStrengthHip    = 0.10f;
		D.SlowInnerStrengthAds    = 0.f;    D.SlowOuterStrengthAds    = 0.20f;
		D.SlowLerpInRate           = 8.f;   D.SlowLerpOutRate          = 4.f;
		D.SlowMinRotationRate      = 0.f;
		return D;
	}();

	static const TArray<FMYSTAimAssistPresetData> Defaults = { Disabled, Low, Moderate, Strong };
}

// ── UMYSTAimAssistPresetsDataAsset ────────────────────────────────────────────

UMYSTAimAssistPresetsDataAsset::UMYSTAimAssistPresetsDataAsset()
{
	// Populate with built-in defaults so the asset works out of the box
	// even before the designer edits it in the Unreal Editor.
	Presets = MYSTAimAssist::Defaults;
}

const FMYSTAimAssistPresetData& UMYSTAimAssistPresetsDataAsset::GetPresetData(EMYSTAimAssistPreset Preset) const
{
	const int32 Idx = static_cast<int32>(Preset);
	if (Presets.IsValidIndex(Idx))
	{
		return Presets[Idx];
	}

	// Fallback: return Moderate defaults so the game never crashes on an invalid index.
	return MYSTAimAssist::Moderate;
}


