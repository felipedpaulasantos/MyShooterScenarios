// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/LyraBlockHealthBlueprintLibrary.h"

#include "Character/LyraBlockHealthInterface.h"
#include "Character/LyraBlockHealthLogicComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

#include "Logging/LogMacros.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraBlockHealthBlueprintLibrary)

DEFINE_LOG_CATEGORY_STATIC(LogLyraBlockHealthBPLib, Log, All);

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

	// Common case: UI passes a controller/player state; resolve to the pawn first.
	if (const AController* Controller = Cast<AController>(Object))
	{
		if (const APawn* Pawn = Controller->GetPawn())
		{
			Object = Pawn;
		}
	}

	// If it's an actor, prefer a component that implements the interface.
	if (const AActor* Actor = Cast<AActor>(Object))
	{
		// Prefer the intended logic component if it actually implements the interface.
		if (const ULyraBlockHealthLogicComponent* Logic = Actor->FindComponentByClass<ULyraBlockHealthLogicComponent>())
		{
			if (Logic->GetClass()->ImplementsInterface(ULyraBlockHealthInterface::StaticClass()))
			{
				return Logic;
			}
		}

		// Fallback: any component on the actor that implements the interface.
		const TSet<UActorComponent*> Components = Actor->GetComponents();
		for (UActorComponent* Component : Components)
		{
			if (Component && Component->GetClass()->ImplementsInterface(ULyraBlockHealthInterface::StaticClass()))
			{
				return Component;
			}
		}
	}

	// If the object itself implements it, use it.
	if (Object->GetClass()->ImplementsInterface(ULyraBlockHealthInterface::StaticClass()))
	{
		return Object;
	}

	return nullptr;
}

int32 ULyraBlockHealthBlueprintLibrary::GetCurrentHealthBlocks(const UObject* Object)
{
	const UObject* Resolved = ResolveObjectForBlockHealth(Object);
	if (Resolved)
	{
		return ILyraBlockHealthInterface::Execute_GetCurrentBlocks(Resolved);
	}

	UE_LOG(LogLyraBlockHealthBPLib, Warning, TEXT("GetCurrentHealthBlocks: No ILyraBlockHealthInterface found for object '%s'"), *GetNameSafe(Object));
	return 0;
}

int32 ULyraBlockHealthBlueprintLibrary::GetMaxHealthBlocks(const UObject* Object)
{
	const UObject* Resolved = ResolveObjectForBlockHealth(Object);
	if (Resolved)
	{
		return ILyraBlockHealthInterface::Execute_GetMaxBlocks(Resolved);
	}

	UE_LOG(LogLyraBlockHealthBPLib, Warning, TEXT("GetMaxHealthBlocks: No ILyraBlockHealthInterface found for object '%s'"), *GetNameSafe(Object));
	return 0;
}

float ULyraBlockHealthBlueprintLibrary::GetHealthBlocksNormalized(const UObject* Object)
{
	const UObject* Resolved = ResolveObjectForBlockHealth(Object);
	if (Resolved)
	{
		return ILyraBlockHealthInterface::Execute_GetBlocksNormalized(Resolved);
	}

	UE_LOG(LogLyraBlockHealthBPLib, Warning, TEXT("GetHealthBlocksNormalized: No ILyraBlockHealthInterface found for object '%s'"), *GetNameSafe(Object));
	return 0.0f;
}
