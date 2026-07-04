using UnrealBuildTool;
using System.Collections.Generic;

public class MY_SHOOTEREditorTarget : TargetRules
{
	public MY_SHOOTEREditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		CppStandard = CppStandardVersion.Cpp20;

		// Required for installed-engine workflows when overriding build settings.
		bOverrideBuildEnvironment = true;

		ExtraModuleNames.AddRange(new string[] { "LyraGame", "LyraEditor" });

		MY_SHOOTERTargetSettings.ApplySharedTargetSettings(this);

		// Keep editor/live coding memory pressure manageable.
		bUseUnityBuild = false;
		bUseAdaptiveUnityBuild = false;

		EnablePlugins.Add("RemoteSession");
	}
}

