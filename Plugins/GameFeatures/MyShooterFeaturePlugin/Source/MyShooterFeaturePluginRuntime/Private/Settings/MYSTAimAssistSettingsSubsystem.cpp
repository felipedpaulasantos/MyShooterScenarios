// Copyright Epic Games, Inc. All Rights Reserved.

#include "Settings/MYSTAimAssistSettingsSubsystem.h"
#include "Settings/LyraSettingsShared.h"
#include "Player/LyraLocalPlayer.h"
#include "Input/AimAssistInputModifier.h"
#include "ScalableFloat.h"
#include "Engine/GameInstance.h"
#include "UObject/UObjectIterator.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MYSTAimAssistSettingsSubsystem)

// ── USubsystem lifecycle ──────────────────────────────────────────────────────

void UMYSTAimAssistSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Set the default DataAsset path; designers can override in the editor or Blueprint.
	if (PresetsDataAsset.IsNull())
	{
		PresetsDataAsset = TSoftObjectPtr<UMYSTAimAssistPresetsDataAsset>(
			FSoftObjectPath(TEXT("/MyShooterFeaturePlugin/Settings/DA_AimAssistPresets.DA_AimAssistPresets")));
	}

	// Bind to shared settings so we re-apply whenever the user changes the preset
	// in the settings menu (the OnSettingChanged broadcast covers all setting mutations).
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const ULyraLocalPlayer* LP = Cast<ULyraLocalPlayer>(GI->GetLocalPlayerByIndex(0)))
		{
			if (ULyraSettingsShared* SharedSettings = LP->GetSharedSettings())
			{
				SettingsChangedHandle = SharedSettings->OnSettingChanged.AddUObject(
					this, &UMYSTAimAssistSettingsSubsystem::OnSettingsChanged);
			}
		}
	}
}

void UMYSTAimAssistSettingsSubsystem::Deinitialize()
{
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const ULyraLocalPlayer* LP = Cast<ULyraLocalPlayer>(GI->GetLocalPlayerByIndex(0)))
		{
			if (ULyraSettingsShared* SharedSettings = LP->GetSharedSettings())
			{
				SharedSettings->OnSettingChanged.Remove(SettingsChangedHandle);
			}
		}
	}

	Super::Deinitialize();
}

// ── Public API ────────────────────────────────────────────────────────────────

UMYSTAimAssistPresetsDataAsset* UMYSTAimAssistSettingsSubsystem::GetPresetsDataAsset() const
{
	return PresetsDataAsset.LoadSynchronous();
}

void UMYSTAimAssistSettingsSubsystem::ApplyCurrentPreset()
{
	UMYSTAimAssistPresetsDataAsset* DA = GetPresetsDataAsset();
	if (!DA)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[MYSTAimAssistSettingsSubsystem] PresetsDataAsset is not set or failed to load."
			     " Assign /MyShooterFeaturePlugin/Settings/DA_AimAssistPresets or create the asset."));
		return;
	}

	// Read current preset from shared settings (default to Moderate if not found).
	EMYSTAimAssistPreset CurrentPreset = EMYSTAimAssistPreset::Moderate;
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const ULyraLocalPlayer* LP = Cast<ULyraLocalPlayer>(GI->GetLocalPlayerByIndex(0)))
		{
			if (const ULyraSettingsShared* SharedSettings = LP->GetSharedSettings())
			{
				CurrentPreset = SharedSettings->GetAimAssistPreset();
			}
		}
	}

	const FMYSTAimAssistPresetData& PresetData = DA->GetPresetData(CurrentPreset);
	ApplyPresetData(PresetData);
}

// ── Private helpers ───────────────────────────────────────────────────────────

void UMYSTAimAssistSettingsSubsystem::OnSettingsChanged(ULyraSettingsShared* /*SharedSettings*/)
{
	ApplyCurrentPreset();
}

void UMYSTAimAssistSettingsSubsystem::ApplyPresetData(const FMYSTAimAssistPresetData& Data) const
{
	// Iterate every live UAimAssistInputModifier instance.
	// Using TObjectIterator avoids coupling to a specific IMC or Enhanced Input internal API.
	// CDO / archetype objects are skipped. In a single-player game this is reliable and cheap.
	int32 ModifiersPatched = 0;
	for (TObjectIterator<UAimAssistInputModifier> It; It; ++It)
	{
		UAimAssistInputModifier* AimAssist = *It;
		if (!IsValid(AimAssist) || AimAssist->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
		{
			continue;
		}

		FAimAssistSettings& S = AimAssist->Settings;

		// Pull strengths
		S.PullInnerStrengthHip  = FScalableFloat(Data.PullInnerStrengthHip);
		S.PullOuterStrengthHip  = FScalableFloat(Data.PullOuterStrengthHip);
		S.PullInnerStrengthAds  = FScalableFloat(Data.PullInnerStrengthAds);
		S.PullOuterStrengthAds  = FScalableFloat(Data.PullOuterStrengthAds);
		S.PullLerpInRate        = FScalableFloat(Data.PullLerpInRate);
		S.PullLerpOutRate       = FScalableFloat(Data.PullLerpOutRate);
		S.PullMaxRotationRate   = FScalableFloat(Data.PullMaxRotationRate);

		// Slow strengths
		S.SlowInnerStrengthHip  = FScalableFloat(Data.SlowInnerStrengthHip);
		S.SlowOuterStrengthHip  = FScalableFloat(Data.SlowOuterStrengthHip);
		S.SlowInnerStrengthAds  = FScalableFloat(Data.SlowInnerStrengthAds);
		S.SlowOuterStrengthAds  = FScalableFloat(Data.SlowOuterStrengthAds);
		S.SlowLerpInRate        = FScalableFloat(Data.SlowLerpInRate);
		S.SlowLerpOutRate       = FScalableFloat(Data.SlowLerpOutRate);
		S.SlowMinRotationRate   = FScalableFloat(Data.SlowMinRotationRate);

		++ModifiersPatched;
	}

	UE_LOG(LogTemp, Verbose,
		TEXT("[MYSTAimAssistSettingsSubsystem] ApplyPresetData — patched %d modifier(s)."),
		ModifiersPatched);
}

