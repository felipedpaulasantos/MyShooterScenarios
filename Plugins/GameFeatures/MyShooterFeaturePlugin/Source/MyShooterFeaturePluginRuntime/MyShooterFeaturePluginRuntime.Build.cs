// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MyShooterFeaturePluginRuntime : ModuleRules
{
	public MyShooterFeaturePluginRuntime(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[]
			{
				// ... add public include paths required here ...
			}
		);


		PrivateIncludePaths.AddRange(
			new string[]
			{
				// ... add another private include paths required here ...
			}
		);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"GameplayTags",       // FGameplayTag in public APIs
				"GameplayAbilities",  // UGameplayEffect, UAttributeSet, FScalableFloat — used in public fragment headers
				"Engine",             // UAnimMontage, UActorComponent base types
				"ModularGameplay",    // UControllerComponent (used by UMYSTInventoryCapacityComponent)
				"LyraGame",           // ULyraInventoryItemFragment base class used in public fragment headers
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Slate",
				"SlateCore",
				// UGameplayTask::ReadyForActivation() is in GameplayTasks (not exported
				// transitively by GameplayAbilities for linking purposes).
				"GameplayTasks",
				// UGameplayMessageSubsystem used by UMYSTWeaponBarComponent broadcasts.
				"GameplayMessageRuntime",
			}
		);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
}