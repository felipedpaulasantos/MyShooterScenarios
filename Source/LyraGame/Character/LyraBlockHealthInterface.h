// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"

#include "LyraBlockHealthInterface.generated.h"

UINTERFACE(BlueprintType)
class LYRAGAME_API ULyraBlockHealthInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for systems/UI that want to consume health-as-blocks regardless of how it's implemented.
 *
 * IMPORTANT: This does not replace GAS Health/MaxHealth. It's a read-only view + settings surface.
 */
class LYRAGAME_API ILyraBlockHealthInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Lyra|Health|Blocks")
	bool IsBlockHealthEnabled() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Lyra|Health|Blocks")
	float GetBlockSize() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Lyra|Health|Blocks")
	float GetTwoBlockDamageThreshold() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Lyra|Health|Blocks")
	int32 GetMaxDamageBlocksPerHit() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Lyra|Health|Blocks")
	int32 GetCurrentBlocks() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Lyra|Health|Blocks")
	int32 GetMaxBlocks() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Lyra|Health|Blocks")
	float GetBlocksNormalized() const;
};

