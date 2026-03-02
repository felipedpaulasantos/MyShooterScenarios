// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "Character/LyraBlockHealthInterface.h"

#include "LyraBlockHealthLogicComponent.generated.h"

class ULyraAbilitySystemComponent;
class ULyraHealthComponent;
class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FLyraBlockHealthLogic_BlocksChanged, UActorComponent*, LogicComponent, int32, OldBlocks, int32, NewBlocks, AActor*, Instigator);

/**
 * ULyraBlockHealthLogicComponent
 *
 * Drop-in addon for the existing ULyraHealthComponent created by ALyraCharacter.
 *
 * It doesn't modify ULyraHealthComponent (no new methods/vars there).
 * Instead it binds to ULyraHealthComponent delegates and enforces block-based rules
 * server-side by overriding the GAS Health attribute base value.
 */
UCLASS(Blueprintable, Meta = (BlueprintSpawnableComponent))
class LYRAGAME_API ULyraBlockHealthLogicComponent : public UActorComponent, public ILyraBlockHealthInterface
{
	GENERATED_BODY()

public:
	ULyraBlockHealthLogicComponent(const FObjectInitializer& ObjectInitializer);

	/** Enable/disable enforcement of block rules. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Health|Blocks")
	bool bEnableBlockHealth = true;

	/** Size of one block in health units (default: 25). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Health|Blocks", meta = (ClampMin = "0.01"))
	float BlockSize = 25.0f;

	/** Damage <= this threshold consumes 1 block; damage above consumes 2 blocks (default: 50). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Health|Blocks", meta = (ClampMin = "0.0"))
	float TwoBlockDamageThreshold = 50.0f;

	/** Maximum number of blocks a single damage event can remove (default: 2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Health|Blocks", meta = (ClampMin = "1"))
	int32 MaxDamageBlocksPerHit = 2;

	/** Fired when current block count changes (derived from Health/MaxHealth). */
	UPROPERTY(BlueprintAssignable, Category = "Lyra|Health|Blocks")
	FLyraBlockHealthLogic_BlocksChanged OnBlocksChanged;

	/** Optional: set health to full blocks on BeginPlay/when found (useful if you want always-multiple-of-block). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Health|Blocks")
	bool bQuantizeOnBeginPlay = true;

public:
	//~UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of UActorComponent

	UFUNCTION(BlueprintCallable, Category = "Lyra|Health|Blocks")
	ULyraHealthComponent* GetLyraHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintCallable, Category = "Lyra|Health|Blocks")
	bool IsServerAuthority() const;

	/** Force a re-quantization based on current health/max health. Server-only. */
	UFUNCTION(BlueprintCallable, Category = "Lyra|Health|Blocks")
	void QuantizeNow();

public:
	// ILyraBlockHealthInterface
	virtual bool IsBlockHealthEnabled_Implementation() const override { return bEnableBlockHealth; }
	virtual float GetBlockSize_Implementation() const override { return BlockSize; }
	virtual float GetTwoBlockDamageThreshold_Implementation() const override { return TwoBlockDamageThreshold; }
	virtual int32 GetMaxDamageBlocksPerHit_Implementation() const override { return MaxDamageBlocksPerHit; }
	virtual int32 GetCurrentBlocks_Implementation() const override;
	virtual int32 GetMaxBlocks_Implementation() const override;
	virtual float GetBlocksNormalized_Implementation() const override;

protected:
	UFUNCTION()
	void HandleHealthChanged(ULyraHealthComponent* InHealthComponent, float OldValue, float NewValue, AActor* Instigator);

	UFUNCTION()
	void HandleMaxHealthChanged(ULyraHealthComponent* InHealthComponent, float OldValue, float NewValue, AActor* Instigator);

	void TryBindToHealthComponent();
	ULyraAbilitySystemComponent* GetLyraASC() const;

	int32 HealthToBlocks_RoundUp(float Health) const;
	float BlocksToHealth(int32 Blocks) const;

	void ApplyQuantizedHealthIfNeeded(float OldHealth, float NewHealth);

private:
	UPROPERTY(Transient)
	TObjectPtr<ULyraHealthComponent> HealthComponent;

	bool bApplyingQuantization = false;
};

