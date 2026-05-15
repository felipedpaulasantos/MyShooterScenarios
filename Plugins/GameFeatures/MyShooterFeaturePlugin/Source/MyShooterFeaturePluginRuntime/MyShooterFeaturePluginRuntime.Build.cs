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
				// UEnvQueryTest (UMYSTEnvQueryTest_ClaimedSpot), AAIController,
				// UBTTaskNode, UBehaviorTreeComponent
				"AIModule",
				// UNavigationSystemV1::ProjectPointToNavigation (BTTask_FindPeekLocation)
				"NavigationSystem",
				// ULoadingScreenManager + ILoadingProcessInterface (MYSTLevelLoadingSubsystem)
				"CommonLoadingScreen",
				// FShaderPipelineCache::NumPrecompilesRemaining() (MYSTLevelLoadingSubsystem)
				"RenderCore",
				// IMoviePlayer + FLoadingScreenAttributes — keeps Slate ticking during
				// the synchronous LoadMap/FlushAsyncLoading block (MYSTLevelLoadingSubsystem)
				"MoviePlayer",
				// UWorldPartitionSubsystem header is part of the Engine module (already a public dep).
				// No extra module needed.
				// UAimAssistInputModifier + FAimAssistSettings (MYSTAimAssistSettingsSubsystem)
				"ShooterCoreRuntime",
				// UEnhancedPlayerInput, UEnhancedInputLocalPlayerSubsystem (MYSTAimAssistSettingsSubsystem)
				"EnhancedInput",
				// LyraSettingsShared.h includes SubtitleDisplayOptions.h from GameSubtitles,
				// which is a Private dep of LyraGame and therefore not transitive to this plugin.
				"GameSubtitles",
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