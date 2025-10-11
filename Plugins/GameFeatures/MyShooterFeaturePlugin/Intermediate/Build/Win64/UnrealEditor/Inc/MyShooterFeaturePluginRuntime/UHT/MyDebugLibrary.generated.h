// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "MyDebugLibrary.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class UObject;
struct FLinearColor;
#ifdef MYSHOOTERFEATUREPLUGINRUNTIME_MyDebugLibrary_generated_h
#error "MyDebugLibrary.generated.h already included, missing '#pragma once' in MyDebugLibrary.h"
#endif
#define MYSHOOTERFEATUREPLUGINRUNTIME_MyDebugLibrary_generated_h

#define FID_UnrealProjects_MyShooterScenarios_Plugins_GameFeatures_MyShooterFeaturePlugin_Source_MyShooterFeaturePluginRuntime_Public_MyDebugLibrary_h_15_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execPrintStringAdvanced);


#define FID_UnrealProjects_MyShooterScenarios_Plugins_GameFeatures_MyShooterFeaturePlugin_Source_MyShooterFeaturePluginRuntime_Public_MyDebugLibrary_h_15_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUMyDebugLibrary(); \
	friend struct Z_Construct_UClass_UMyDebugLibrary_Statics; \
public: \
	DECLARE_CLASS(UMyDebugLibrary, UBlueprintFunctionLibrary, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/MyShooterFeaturePluginRuntime"), NO_API) \
	DECLARE_SERIALIZER(UMyDebugLibrary)


#define FID_UnrealProjects_MyShooterScenarios_Plugins_GameFeatures_MyShooterFeaturePlugin_Source_MyShooterFeaturePluginRuntime_Public_MyDebugLibrary_h_15_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UMyDebugLibrary(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UMyDebugLibrary(UMyDebugLibrary&&); \
	UMyDebugLibrary(const UMyDebugLibrary&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UMyDebugLibrary); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UMyDebugLibrary); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UMyDebugLibrary) \
	NO_API virtual ~UMyDebugLibrary();


#define FID_UnrealProjects_MyShooterScenarios_Plugins_GameFeatures_MyShooterFeaturePlugin_Source_MyShooterFeaturePluginRuntime_Public_MyDebugLibrary_h_12_PROLOG
#define FID_UnrealProjects_MyShooterScenarios_Plugins_GameFeatures_MyShooterFeaturePlugin_Source_MyShooterFeaturePluginRuntime_Public_MyDebugLibrary_h_15_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_UnrealProjects_MyShooterScenarios_Plugins_GameFeatures_MyShooterFeaturePlugin_Source_MyShooterFeaturePluginRuntime_Public_MyDebugLibrary_h_15_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_UnrealProjects_MyShooterScenarios_Plugins_GameFeatures_MyShooterFeaturePlugin_Source_MyShooterFeaturePluginRuntime_Public_MyDebugLibrary_h_15_INCLASS_NO_PURE_DECLS \
	FID_UnrealProjects_MyShooterScenarios_Plugins_GameFeatures_MyShooterFeaturePlugin_Source_MyShooterFeaturePluginRuntime_Public_MyDebugLibrary_h_15_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> MYSHOOTERFEATUREPLUGINRUNTIME_API UClass* StaticClass<class UMyDebugLibrary>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_UnrealProjects_MyShooterScenarios_Plugins_GameFeatures_MyShooterFeaturePlugin_Source_MyShooterFeaturePluginRuntime_Public_MyDebugLibrary_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
