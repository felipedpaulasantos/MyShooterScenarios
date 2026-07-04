using UnrealBuildTool;
using System;
using System.IO;
using EpicGames.Core;
using System.Collections.Generic;
using UnrealBuildBase;

public class MY_SHOOTERTarget : TargetRules
{
	public MY_SHOOTERTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		CppStandard = CppStandardVersion.Cpp20;

		ExtraModuleNames.AddRange(new string[] { "LyraGame" });

		MY_SHOOTERTargetSettings.ApplySharedTargetSettings(this);
	}
}

public static class MY_SHOOTERTargetSettings
{
	// ...existing code copied from the legacy Lyra target helper...
	private static bool bHasWarnedAboutShared = false;

	internal static void ApplySharedTargetSettings(TargetRules Target)
	{
		bool bIsTest = Target.Configuration == UnrealTargetConfiguration.Test;
		bool bIsShipping = Target.Configuration == UnrealTargetConfiguration.Shipping;
		bool bIsDedicatedServer = Target.Type == TargetType.Server;
		if (Target.BuildEnvironment == TargetBuildEnvironment.Unique)
		{
			Target.ShadowVariableWarningLevel = WarningLevel.Error;

			Target.bUseLoggingInShipping = true;

			if (bIsShipping && !bIsDedicatedServer)
			{
				Target.bDisableUnverifiedCertificates = true;
			}

			if (bIsShipping || bIsTest)
			{
				Target.bAllowGeneratedIniWhenCooked = false;
				Target.bAllowNonUFSIniWhenCooked = false;
			}

			if (Target.Type != TargetType.Editor)
			{
				Target.DisablePlugins.Add("OpenImageDenoise");
			}

			ConfigureGameFeaturePlugins(Target);
		}
		else if (Target.Type == TargetType.Editor)
		{
			ConfigureGameFeaturePlugins(Target);
		}
		else if (!bHasWarnedAboutShared)
		{
			bHasWarnedAboutShared = true;
			Log.WriteLine(LogEventType.Warning, "LyraGameEOS and dynamic target options are disabled when packaging from an installed version of the engine");
		}
	}

	static public bool ShouldEnableAllGameFeaturePlugins(TargetRules Target)
	{
		if (Target.Type == TargetType.Editor)
		{
		}

		bool bIsBuildMachine = (Environment.GetEnvironmentVariable("IsBuildMachine") == "1");
		if (bIsBuildMachine)
		{
		}

		return false;
	}

	static public void ConfigureGameFeaturePlugins(TargetRules Target)
	{
		Log.WriteLine(LogEventType.Console, "Compiling GameFeaturePlugins in branch {0}", Target.Version.BranchName);

		bool bBuildAllGameFeaturePlugins = ShouldEnableAllGameFeaturePlugins(Target);
		List<FileReference> CombinedPluginList = new List<FileReference>();
		List<DirectoryReference> GameFeaturePluginRoots = Unreal.GetExtensionDirs(Target.ProjectFile.Directory, Path.Combine("Plugins", "GameFeatures"));
		foreach (DirectoryReference SearchDir in GameFeaturePluginRoots)
		{
			CombinedPluginList.AddRange(PluginsBase.EnumeratePlugins(SearchDir));
		}

		if (CombinedPluginList.Count > 0)
		{
			Dictionary<string, JsonObject> AllPluginRootJsonObjectsByName = new Dictionary<string, JsonObject>();
			Dictionary<string, List<string>> AllPluginReferencesByName = new Dictionary<string, List<string>>();

			foreach (FileReference PluginFile in CombinedPluginList)
			{
				if (PluginFile != null && FileReference.Exists(PluginFile))
				{
					bool bEnabled = false;
					bool bForceDisabled = false;
					try
					{
						JsonObject RawObject = JsonObject.Read(PluginFile);
						AllPluginRootJsonObjectsByName.Add(PluginFile.GetFileNameWithoutExtension(), RawObject);

						bool bEnabledByDefault = false;
						if (!RawObject.TryGetBoolField("EnabledByDefault", out bEnabledByDefault) || bEnabledByDefault == true)
						{
						}

						bool bExplicitlyLoaded = false;
						if (!RawObject.TryGetBoolField("ExplicitlyLoaded", out bExplicitlyLoaded) || bExplicitlyLoaded == false)
						{
							Log.WriteLine(LogEventType.Warning, "GameFeaturePlugin {0}, does not set ExplicitlyLoaded to true. This is required for GameFeaturePlugins.", PluginFile.GetFileNameWithoutExtension());
						}

						if (bBuildAllGameFeaturePlugins)
						{
							bEnabled = true;
						}

						bool bEditorOnly = false;
						if (RawObject.TryGetBoolField("EditorOnly", out bEditorOnly))
						{
							if (bEditorOnly && (Target.Type != TargetType.Editor) && !bBuildAllGameFeaturePlugins)
							{
								bForceDisabled = true;
							}
						}

						string RestrictToBranch;
						if (RawObject.TryGetStringField("RestrictToBranch", out RestrictToBranch))
						{
							if (!Target.Version.BranchName.Equals(RestrictToBranch, StringComparison.OrdinalIgnoreCase))
							{
								bForceDisabled = true;
								Log.WriteLine(LogEventType.Verbose, "GameFeaturePlugin {0} was marked as restricted to other branches. Disabling.", PluginFile.GetFileNameWithoutExtension());
							}
							else
							{
								Log.WriteLine(LogEventType.Verbose, "GameFeaturePlugin {0} was marked as restricted to this branch. Leaving enabled.", PluginFile.GetFileNameWithoutExtension());
							}
						}

						bool bNeverBuild = false;
						if (RawObject.TryGetBoolField("NeverBuild", out bNeverBuild) && bNeverBuild)
						{
							bForceDisabled = true;
							Log.WriteLine(LogEventType.Verbose, "GameFeaturePlugin {0} was marked as NeverBuild, disabling.", PluginFile.GetFileNameWithoutExtension());
						}

						JsonObject[] PluginReferencesArray;
						if (RawObject.TryGetObjectArrayField("Plugins", out PluginReferencesArray))
						{
							foreach (JsonObject ReferenceObject in PluginReferencesArray)
							{
								bool bRefEnabled = false;
								if (ReferenceObject.TryGetBoolField("Enabled", out bRefEnabled) && bRefEnabled == true)
								{
									string PluginReferenceName;
									if (ReferenceObject.TryGetStringField("Name", out PluginReferenceName))
									{
										string ReferencerName = PluginFile.GetFileNameWithoutExtension();
										if (!AllPluginReferencesByName.ContainsKey(ReferencerName))
										{
											AllPluginReferencesByName[ReferencerName] = new List<string>();
										}
										AllPluginReferencesByName[ReferencerName].Add(PluginReferenceName);
									}
								}
							}
						}
					}
					catch (Exception ParseException)
					{
						Log.WriteLine(LogEventType.Warning, "Failed to parse GameFeaturePlugin file {0}, disabling. Exception: {1}", PluginFile.GetFileNameWithoutExtension(), ParseException.Message);
						bForceDisabled = true;
					}

					if (bForceDisabled)
					{
						bEnabled = false;
					}

					Log.WriteLine(LogEventType.Verbose, "ConfigureGameFeaturePlugins() has decided to {0} feature {1}", bEnabled ? "enable" : (bForceDisabled ? "disable" : "ignore"), PluginFile.GetFileNameWithoutExtension());

					if (bEnabled)
					{
						Target.EnablePlugins.Add(PluginFile.GetFileNameWithoutExtension());
					}
					else if (bForceDisabled)
					{
						Target.DisablePlugins.Add(PluginFile.GetFileNameWithoutExtension());
					}
				}
			}
		}
	}
}

