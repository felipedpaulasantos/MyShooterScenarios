// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class LyraEditorTarget : TargetRules
{
	public LyraEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		CppStandard = CppStandardVersion.Cpp20;
		
		// Required for UE 5.4 when using installed engine and modifying build settings
		bOverrideBuildEnvironment = true;

		ExtraModuleNames.AddRange(new string[] { "LyraGame", "LyraEditor" });

		LyraGameTarget.ApplySharedLyraTargetSettings(this);

		// Live Coding builds can be very PCH-heavy and hit MSVC heap/pagefile limits.
		// Disabling unity builds tends to reduce peak PCH pressure for iterative compiles.
		bUseUnityBuild = false;
		bUseAdaptiveUnityBuild = false;

		// Avoid distributed build executors during Live Coding (helps stability when memory is tight).
		// (Not all target rule flags exist in all UE versions; leave executor selection to UBT defaults.)

		// This is used for touch screen development along with the "Unreal Remote 2" app
		EnablePlugins.Add("RemoteSession");
	}
}
