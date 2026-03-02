// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "LyraBlockHealthBlueprintLibrary.generated.h"

class AActor;
class UObject;
class ULyraBlockHealthLogicComponent;

/** Convenience Blueprint helpers for block-based health UI. */
UCLASS()
class LYRAGAME_API ULyraBlockHealthBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Finds the first ULyraBlockHealthLogicComponent on the actor (or nullptr). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Health|Blocks")
	static ULyraBlockHealthLogicComponent* FindBlockHealthLogicComponent(const AActor* Actor);

	/** True if the provided object (or attached logic component) implements ILyraBlockHealthInterface. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Health|Blocks")
	static bool ImplementsBlockHealthInterface(const UObject* Object);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Health|Blocks")
	static int32 GetCurrentHealthBlocks(const UObject* Object);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Health|Blocks")
	static int32 GetMaxHealthBlocks(const UObject* Object);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|Health|Blocks")
	static float GetHealthBlocksNormalized(const UObject* Object);
};
