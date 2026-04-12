// Copyright MyShooterScenarios. All Rights Reserved.

#include "MusicManagerSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MusicManagerSubsystem)

void UMusicManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	K2_OnInitialize();
}

void UMusicManagerSubsystem::Deinitialize()
{
	K2_OnDeinitialize();
	Super::Deinitialize();
}

UWorld* UMusicManagerSubsystem::GetWorld() const
{
	// GameInstance always has a valid world; this is what makes BP nodes "just work"
	// without needing to manually wire the World Context Object pin.
	if (const UGameInstance* GI = GetGameInstance())
	{
		return GI->GetWorld();
	}
	return nullptr;
}

