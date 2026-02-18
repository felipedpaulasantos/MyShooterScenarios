// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableIconInterface.generated.h"

/**
 * Interface for actors that can show/hide an interactable icon.
 *
 * Blueprint-first: actors can implement this in BP or C++.
 */
UINTERFACE(BlueprintType)
class LYRAGAME_API UInteractableIconInterface : public UInterface
{
	GENERATED_BODY()
};

class LYRAGAME_API IInteractableIconInterface
{
	GENERATED_BODY()

public:
	/** Minimum distance (in cm) from the local player camera/pawn to start showing the icon. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Icon")
	float GetMinimumDistanceToShowIcon() const;

	/** Show/hide the icon. Should be client/UI-only. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Icon")
	void SetIconVisibility(bool bVisible);

	/** Optional: override where the icon/trace should aim (defaults to ActorLocation if not overridden). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Icon")
	FVector GetIconWorldLocation() const;
};

