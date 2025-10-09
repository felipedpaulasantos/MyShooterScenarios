#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_Tick.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTickDelegate, float, DeltaTime);

/**
 * Custom ability task that ticks every frame.
 */
UCLASS()
class UAbilityTask_Tick : public UAbilityTask
{
    GENERATED_BODY()

public:
    // This is the event you’ll bind to in the Gameplay Ability Blueprint
    UPROPERTY(BlueprintAssignable)
    FOnTickDelegate OnTick;

    // This function starts the task and will be callable from Blueprints
    UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (DisplayName = "Start Tick Task", DefaultToSelf = "OwningAbility"))
    static UAbilityTask_Tick* StartTickTask(UGameplayAbility* OwningAbility);

protected:
    // Called every frame
    virtual void TickTask(float DeltaTime) override;

    // Activate the task
    virtual void Activate() override;

    // Called when the task ends or is destroyed
    virtual void OnDestroy(bool AbilityEnded) override;
};