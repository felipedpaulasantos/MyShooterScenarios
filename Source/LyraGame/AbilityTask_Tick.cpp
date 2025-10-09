#include "AbilityTask_Tick.h"

UAbilityTask_Tick* UAbilityTask_Tick::StartTickTask(UGameplayAbility* OwningAbility)
{
    // Create the task
    UAbilityTask_Tick* Task = NewAbilityTask<UAbilityTask_Tick>(OwningAbility);
    return Task;
}

void UAbilityTask_Tick::Activate()
{
    SetWaitingOnAvatar();  // Register this task to be managed by the avatar (player/character)
    // Set this task to tick
    bTickingTask = true;
}

void UAbilityTask_Tick::TickTask(float DeltaTime)
{
    if (Ability)
    {
        // Broadcast the OnTick delegate to notify Blueprints every frame
        OnTick.Broadcast(DeltaTime);
    }
}

void UAbilityTask_Tick::OnDestroy(bool AbilityEnded)
{
    Super::OnDestroy(AbilityEnded);
    // Cleanup any resources here when the task is destroyed
    bTickingTask = false;
}
