// Copyright MyShooterScenarios. All Rights Reserved.
#include "CarChase/MYSTChunkDefinition.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(MYSTChunkDefinition)
FString UMYSTChunkDefinition::GetDisplayName() const
{
if (!ChunkDisplayName.IsNone())
{
return ChunkDisplayName.ToString();
}
return GetName();
}
