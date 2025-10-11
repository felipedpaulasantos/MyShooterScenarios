// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "MyShooterFeaturePluginRuntime/Public/MyDebugLibrary.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeMyDebugLibrary() {}

// Begin Cross Module References
COREUOBJECT_API UClass* Z_Construct_UClass_UObject_NoRegister();
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FLinearColor();
ENGINE_API UClass* Z_Construct_UClass_UBlueprintFunctionLibrary();
MYSHOOTERFEATUREPLUGINRUNTIME_API UClass* Z_Construct_UClass_UMyDebugLibrary();
MYSHOOTERFEATUREPLUGINRUNTIME_API UClass* Z_Construct_UClass_UMyDebugLibrary_NoRegister();
UPackage* Z_Construct_UPackage__Script_MyShooterFeaturePluginRuntime();
// End Cross Module References

// Begin Class UMyDebugLibrary Function PrintStringAdvanced
struct Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics
{
	struct MyDebugLibrary_eventPrintStringAdvanced_Parms
	{
		const UObject* WorldContextObject;
		FString InString;
		bool bPrintToScreen;
		bool bPrintToLog;
		FLinearColor TextColor;
		float Duration;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "AdvancedDisplay", "bPrintToScreen,bPrintToLog,TextColor,Duration" },
		{ "Category", "My Debug" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Imprime uma string na tela com o nome da classe e o timestamp atual.\n\x09 * @param WorldContextObject Fornece o contexto do mundo e do objeto que est\xef\xbf\xbd chamando esta fun\xef\xbf\xbd\xef\xbf\xbdo.\n\x09 * @param InString A string a ser impressa.\n\x09 * @param bPrintToScreen Se deve imprimir na tela.\n\x09 * @param bPrintToLog Se deve imprimir no log de sa\xef\xbf\xbd""da.\n\x09 * @param TextColor A cor do texto na tela.\n\x09 * @param Duration A em segundos.\n\x09*/" },
#endif
		{ "CPP_Default_bPrintToLog", "true" },
		{ "CPP_Default_bPrintToScreen", "true" },
		{ "CPP_Default_Duration", "2.000000" },
		{ "CPP_Default_InString", "Hello" },
		{ "CPP_Default_TextColor", "(R=0.000000,G=0.660000,B=1.000000,A=1.000000)" },
		{ "DevelopmentOnly", "" },
		{ "Keywords", "log print" },
		{ "ModuleRelativePath", "Public/MyDebugLibrary.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Imprime uma string na tela com o nome da classe e o timestamp atual.\n@param WorldContextObject Fornece o contexto do mundo e do objeto que est\xef\xbf\xbd chamando esta fun\xef\xbf\xbd\xef\xbf\xbdo.\n@param InString A string a ser impressa.\n@param bPrintToScreen Se deve imprimir na tela.\n@param bPrintToLog Se deve imprimir no log de sa\xef\xbf\xbd""da.\n@param TextColor A cor do texto na tela.\n@param Duration A em segundos." },
#endif
		{ "WorldContext", "WorldContextObject" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_WorldContextObject_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_InString_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_WorldContextObject;
	static const UECodeGen_Private::FStrPropertyParams NewProp_InString;
	static void NewProp_bPrintToScreen_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bPrintToScreen;
	static void NewProp_bPrintToLog_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bPrintToLog;
	static const UECodeGen_Private::FStructPropertyParams NewProp_TextColor;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Duration;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::NewProp_WorldContextObject = { "WorldContextObject", nullptr, (EPropertyFlags)0x0010000000000082, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(MyDebugLibrary_eventPrintStringAdvanced_Parms, WorldContextObject), Z_Construct_UClass_UObject_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_WorldContextObject_MetaData), NewProp_WorldContextObject_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::NewProp_InString = { "InString", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(MyDebugLibrary_eventPrintStringAdvanced_Parms, InString), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_InString_MetaData), NewProp_InString_MetaData) };
void Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::NewProp_bPrintToScreen_SetBit(void* Obj)
{
	((MyDebugLibrary_eventPrintStringAdvanced_Parms*)Obj)->bPrintToScreen = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::NewProp_bPrintToScreen = { "bPrintToScreen", nullptr, (EPropertyFlags)0x0010040000000080, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(MyDebugLibrary_eventPrintStringAdvanced_Parms), &Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::NewProp_bPrintToScreen_SetBit, METADATA_PARAMS(0, nullptr) };
void Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::NewProp_bPrintToLog_SetBit(void* Obj)
{
	((MyDebugLibrary_eventPrintStringAdvanced_Parms*)Obj)->bPrintToLog = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::NewProp_bPrintToLog = { "bPrintToLog", nullptr, (EPropertyFlags)0x0010040000000080, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(MyDebugLibrary_eventPrintStringAdvanced_Parms), &Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::NewProp_bPrintToLog_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::NewProp_TextColor = { "TextColor", nullptr, (EPropertyFlags)0x0010040000000080, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(MyDebugLibrary_eventPrintStringAdvanced_Parms, TextColor), Z_Construct_UScriptStruct_FLinearColor, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::NewProp_Duration = { "Duration", nullptr, (EPropertyFlags)0x0010040000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(MyDebugLibrary_eventPrintStringAdvanced_Parms, Duration), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::NewProp_WorldContextObject,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::NewProp_InString,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::NewProp_bPrintToScreen,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::NewProp_bPrintToLog,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::NewProp_TextColor,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::NewProp_Duration,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UMyDebugLibrary, nullptr, "PrintStringAdvanced", nullptr, nullptr, Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::PropPointers), sizeof(Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::MyDebugLibrary_eventPrintStringAdvanced_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04822401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::Function_MetaDataParams), Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::MyDebugLibrary_eventPrintStringAdvanced_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UMyDebugLibrary::execPrintStringAdvanced)
{
	P_GET_OBJECT(UObject,Z_Param_WorldContextObject);
	P_GET_PROPERTY(FStrProperty,Z_Param_InString);
	P_GET_UBOOL(Z_Param_bPrintToScreen);
	P_GET_UBOOL(Z_Param_bPrintToLog);
	P_GET_STRUCT(FLinearColor,Z_Param_TextColor);
	P_GET_PROPERTY(FFloatProperty,Z_Param_Duration);
	P_FINISH;
	P_NATIVE_BEGIN;
	UMyDebugLibrary::PrintStringAdvanced(Z_Param_WorldContextObject,Z_Param_InString,Z_Param_bPrintToScreen,Z_Param_bPrintToLog,Z_Param_TextColor,Z_Param_Duration);
	P_NATIVE_END;
}
// End Class UMyDebugLibrary Function PrintStringAdvanced

// Begin Class UMyDebugLibrary
void UMyDebugLibrary::StaticRegisterNativesUMyDebugLibrary()
{
	UClass* Class = UMyDebugLibrary::StaticClass();
	static const FNameNativePtrPair Funcs[] = {
		{ "PrintStringAdvanced", &UMyDebugLibrary::execPrintStringAdvanced },
	};
	FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UMyDebugLibrary);
UClass* Z_Construct_UClass_UMyDebugLibrary_NoRegister()
{
	return UMyDebugLibrary::StaticClass();
}
struct Z_Construct_UClass_UMyDebugLibrary_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n *\n */" },
#endif
		{ "IncludePath", "MyDebugLibrary.h" },
		{ "ModuleRelativePath", "Public/MyDebugLibrary.h" },
	};
#endif // WITH_METADATA
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UMyDebugLibrary_PrintStringAdvanced, "PrintStringAdvanced" }, // 2745738225
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UMyDebugLibrary>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
UObject* (*const Z_Construct_UClass_UMyDebugLibrary_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UBlueprintFunctionLibrary,
	(UObject* (*)())Z_Construct_UPackage__Script_MyShooterFeaturePluginRuntime,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UMyDebugLibrary_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UMyDebugLibrary_Statics::ClassParams = {
	&UMyDebugLibrary::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	nullptr,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	0,
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UMyDebugLibrary_Statics::Class_MetaDataParams), Z_Construct_UClass_UMyDebugLibrary_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UMyDebugLibrary()
{
	if (!Z_Registration_Info_UClass_UMyDebugLibrary.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UMyDebugLibrary.OuterSingleton, Z_Construct_UClass_UMyDebugLibrary_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UMyDebugLibrary.OuterSingleton;
}
template<> MYSHOOTERFEATUREPLUGINRUNTIME_API UClass* StaticClass<UMyDebugLibrary>()
{
	return UMyDebugLibrary::StaticClass();
}
UMyDebugLibrary::UMyDebugLibrary(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UMyDebugLibrary);
UMyDebugLibrary::~UMyDebugLibrary() {}
// End Class UMyDebugLibrary

// Begin Registration
struct Z_CompiledInDeferFile_FID_UnrealProjects_MyShooterScenarios_Plugins_GameFeatures_MyShooterFeaturePlugin_Source_MyShooterFeaturePluginRuntime_Public_MyDebugLibrary_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UMyDebugLibrary, UMyDebugLibrary::StaticClass, TEXT("UMyDebugLibrary"), &Z_Registration_Info_UClass_UMyDebugLibrary, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UMyDebugLibrary), 2203027312U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_UnrealProjects_MyShooterScenarios_Plugins_GameFeatures_MyShooterFeaturePlugin_Source_MyShooterFeaturePluginRuntime_Public_MyDebugLibrary_h_369188055(TEXT("/Script/MyShooterFeaturePluginRuntime"),
	Z_CompiledInDeferFile_FID_UnrealProjects_MyShooterScenarios_Plugins_GameFeatures_MyShooterFeaturePlugin_Source_MyShooterFeaturePluginRuntime_Public_MyDebugLibrary_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_UnrealProjects_MyShooterScenarios_Plugins_GameFeatures_MyShooterFeaturePlugin_Source_MyShooterFeaturePluginRuntime_Public_MyDebugLibrary_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
