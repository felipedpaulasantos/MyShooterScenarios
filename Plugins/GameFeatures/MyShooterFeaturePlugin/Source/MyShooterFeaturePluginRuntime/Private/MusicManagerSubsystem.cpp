// Copyright MyShooterScenarios. All Rights Reserved.

#include "MusicManagerSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MusicManagerSubsystem)

bool UMusicManagerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// If a more-derived subclass (e.g. BP_MusicManagerSubsystem) is loaded,
	// let that one be instantiated instead of this C++ base class.
	// Without this, both classes would be instantiated in the editor (PIE), but
	// in a standalone/Launch Game run the BP class may not be loaded yet — this
	// gate makes the behaviour consistent and avoids a silent "wrong instance" bug.
	TArray<UClass*> DerivedClasses;
	GetDerivedClasses(GetClass(), DerivedClasses, /*bRecursive=*/true);
	for (UClass* DerivedClass : DerivedClasses)
	{
		if (DerivedClass && !DerivedClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
		{
			// A concrete subclass is already loaded — skip the base.
			return false;
		}
	}
	return Super::ShouldCreateSubsystem(Outer);
}

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

