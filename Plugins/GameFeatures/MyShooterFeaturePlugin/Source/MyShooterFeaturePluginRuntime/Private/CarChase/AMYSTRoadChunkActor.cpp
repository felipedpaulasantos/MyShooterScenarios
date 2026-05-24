// Copyright MyShooterScenarios. All Rights Reserved.
#include "CarChase/AMYSTRoadChunkActor.h"
#include "CarChase/AMYSTChaseTrackManager.h"
#include "CarChase/MYSTChunkDefinition.h"
#if WITH_EDITORONLY_DATA
#include "Components/ArrowComponent.h"
#endif
#include UE_INLINE_GENERATED_CPP_BY_NAME(AMYSTRoadChunkActor)
AMYSTRoadChunkActor::AMYSTRoadChunkActor(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
PrimaryActorTick.bCanEverTick = false;
#if WITH_EDITORONLY_DATA
ForwardArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("ForwardArrow"));
if (ForwardArrow)
{
ForwardArrow->ArrowColor     = FColor(0, 200, 255);
ForwardArrow->ArrowSize      = 2.f;
ForwardArrow->bIsEditorOnly  = true;
ForwardArrow->SetupAttachment(GetRootComponent());
}
#endif
}
void AMYSTRoadChunkActor::BeginPlay()
{
Super::BeginPlay();
}
void AMYSTRoadChunkActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
OwningManager   = nullptr;
ChunkDefinition = nullptr;
Super::EndPlay(EndPlayReason);
}
// ─────────────────────────────────────────────────────────────────────────────
// Activation
// ─────────────────────────────────────────────────────────────────────────────
void AMYSTRoadChunkActor::ActivateChunk(AMYSTChaseTrackManager* InManager, const UMYSTChunkDefinition* InDefinition)
{
OwningManager   = InManager;
ChunkDefinition = InDefinition;
K2_OnChunkActivated(InDefinition);
}
void AMYSTRoadChunkActor::DeactivateChunk()
{
K2_OnChunkDeactivated();
OwningManager   = nullptr;
ChunkDefinition = nullptr;
}
// ─────────────────────────────────────────────────────────────────────────────
// Enemy Accounting
// ─────────────────────────────────────────────────────────────────────────────
void AMYSTRoadChunkActor::NotifyEnemyKilled(int32 Count)
{
if (OwningManager)
{
OwningManager->RegisterEnemyKilled(Count);
}
}
void AMYSTRoadChunkActor::NotifyEnemySpawned(int32 Count)
{
if (OwningManager)
{
OwningManager->RegisterEnemySpawned(Count);
}
}
// ─────────────────────────────────────────────────────────────────────────────
// Queries
// ─────────────────────────────────────────────────────────────────────────────
float AMYSTRoadChunkActor::GetChunkLength() const
{
const float DefinitionLength = ChunkDefinition ? ChunkDefinition->ChunkLength : 5000.f;
// Give Blueprint a chance to override the length for procedural chunks.
// BlueprintImplementableEvents return their zero-init default (0.f) when not
// implemented, so we fall back to the definition value in that case.
const float Override = K2_GetChunkLength(DefinitionLength);
return Override > 0.f ? Override : DefinitionLength;
}
FVector AMYSTRoadChunkActor::GetChunkExitWorldLocation() const
{
return GetActorLocation() + GetActorForwardVector() * GetChunkLength();
}
