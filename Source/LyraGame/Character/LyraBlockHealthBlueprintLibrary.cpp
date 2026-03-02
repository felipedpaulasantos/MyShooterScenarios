// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/LyraBlockHealthBlueprintLibrary.h"

#include "Character/LyraBlockHealthInterface.h"
#include "Character/LyraBlockHealthLogicComponent.h"

#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraBlockHealthBlueprintLibrary)

ULyraBlockHealthLogicComponent* ULyraBlockHealthBlueprintLibrary::FindBlockHealthLogicComponent(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<ULyraBlockHealthLogicComponent>() : nullptr;
}

bool ULyraBlockHealthBlueprintLibrary::ImplementsBlockHealthInterface(const UObject* Object)
{
	return Object && Object->GetClass()->ImplementsInterface(ULyraBlockHealthInterface::StaticClass());
}

static const UObject* ResolveObjectForBlockHealth(const UObject* Object)
{
	if (!Object)
	{
		return nullptr;
	}

	// If it's an actor, prefer a component that implements the interface.
	if (const AActor* Actor = Cast<AActor>(Object))
	{
		if (const ULyraBlockHealthLogicComponent* Logic = Actor->FindComponentByClass<ULyraBlockHealthLogicComponent>())
		{
			return Logic;
		}
	}

	return Object;
}

int32 ULyraBlockHealthBlueprintLibrary::GetCurrentHealthBlocks(const UObject* Object)
{
	const UObject* Resolved = ResolveObjectForBlockHealth(Object);
	if (Resolved && Resolved->GetClass()->ImplementsInterface(ULyraBlockHealthInterface::StaticClass()))
	{
		return ILyraBlockHealthInterface::Execute_GetCurrentBlocks(Resolved);
	}
	return 0;
}

int32 ULyraBlockHealthBlueprintLibrary::GetMaxHealthBlocks(const UObject* Object)
{
	const UObject* Resolved = ResolveObjectForBlockHealth(Object);
	if (Resolved && Resolved->GetClass()->ImplementsInterface(ULyraBlockHealthInterface::StaticClass()))
	{
		return ILyraBlockHealthInterface::Execute_GetMaxBlocks(Resolved);
	}
	return 0;
}

float ULyraBlockHealthBlueprintLibrary::GetHealthBlocksNormalized(const UObject* Object)
{
	const UObject* Resolved = ResolveObjectForBlockHealth(Object);
	if (Resolved && Resolved->GetClass()->ImplementsInterface(ULyraBlockHealthInterface::StaticClass()))
	{
		return ILyraBlockHealthInterface::Execute_GetBlocksNormalized(Resolved);
	}
	return 0.0f;
}
