// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LyraGameplayAbility.h"
#include "GameplayTagContainer.h"
#include "LyraGameplayAbility_AutoCover.generated.h"

class UObject;
class AActor;
class UPrimitiveComponent;
struct FFrame;
struct FGameplayAbilityActorInfo;
struct FGameplayTagContainer;
struct FHitResult;

/**
 * ULyraGameplayAbility_AutoCover
 *
 * Gameplay ability used for automatic cover system.
 * Auto-activates when character looks at a mesh with "cover" tag and moves forward.
 * Keeps character attached to cover, allows lateral movement, and aiming when at edges.
 */
UCLASS(Blueprintable, Meta = (ShortTooltip = "Gameplay ability for automatic cover system"))
class LYRAGAME_API ULyraGameplayAbility_AutoCover : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:

	ULyraGameplayAbility_AutoCover(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Called to check if ability should try to activate (can be called from Blueprint or tick)
	UFUNCTION(BlueprintCallable, Category = "Lyra|Ability|Cover")
	bool TryActivateCoverAbility();

protected:

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	
	// Called when ability is given to check for passive monitoring
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	// Check if there's a valid cover mesh in front of the character
	UFUNCTION(BlueprintPure, Category = "Lyra|Ability|Cover")
	bool CheckForCoverInFront(FHitResult& OutHitResult) const;

	// Check if character is moving forward towards cover
	UFUNCTION(BlueprintPure, Category = "Lyra|Ability|Cover")
	bool IsMovingTowardsCover() const;

	// Attach character to cover position
	UFUNCTION(BlueprintCallable, Category = "Lyra|Ability|Cover")
	void AttachToCover(const FHitResult& CoverHit);

	// Detach character from cover
	UFUNCTION(BlueprintCallable, Category = "Lyra|Ability|Cover")
	void DetachFromCover();

	// Update character position along cover based on input
	UFUNCTION(BlueprintCallable, Category = "Lyra|Ability|Cover")
	void UpdateCoverMovement(float DeltaTime);

	// Check if character is at the edge of cover
	UFUNCTION(BlueprintPure, Category = "Lyra|Ability|Cover")
	bool IsAtCoverEdge(bool bCheckRightEdge) const;

	// Get lateral movement input (left/right)
	UFUNCTION(BlueprintPure, Category = "Lyra|Ability|Cover")
	float GetLateralMovementInput() const;

	// Check if can aim from current position
	UFUNCTION(BlueprintPure, Category = "Lyra|Ability|Cover")
	bool CanAimFromCover() const;

	// Called when entering cover
	UFUNCTION(BlueprintImplementableEvent, Category = "Lyra|Ability|Cover", Meta = (DisplayName = "On Enter Cover"))
	void K2_OnEnterCover(const FHitResult& CoverHit);

	// Called when exiting cover
	UFUNCTION(BlueprintImplementableEvent, Category = "Lyra|Ability|Cover", Meta = (DisplayName = "On Exit Cover"))
	void K2_OnExitCover();

	// Called when at cover edge status changes
	UFUNCTION(BlueprintImplementableEvent, Category = "Lyra|Ability|Cover", Meta = (DisplayName = "On Cover Edge Status Changed"))
	void K2_OnCoverEdgeStatusChanged(bool bAtEdge, bool bRightEdge);

	// Called every frame while in cover
	UFUNCTION(BlueprintImplementableEvent, Category = "Lyra|Ability|Cover", Meta = (DisplayName = "On Cover Tick"))
	void K2_OnCoverTick(float DeltaTime);

protected:

	// Enable automatic activation when conditions are met
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Cover Settings|Activation")
	bool bAutoActivate;

	// Check rate for auto activation (seconds between checks)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Cover Settings|Activation")
	float AutoActivationCheckRate;

	// Maximum distance to detect cover
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Cover Settings")
	float CoverDetectionDistance;

	// Trace radius for cover detection
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Cover Settings")
	float CoverDetectionRadius;

	// Tag that identifies cover meshes (Component Tags)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Cover Settings")
	FName CoverTag;

	// Distance from cover wall
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Cover Settings")
	float DistanceFromCover;

	// Speed when moving along cover
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Cover Settings")
	float CoverMovementSpeed;

	// Distance to check for cover edges
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Cover Settings")
	float EdgeDetectionDistance;

	// Minimum forward input to activate cover
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Cover Settings")
	float MinimumForwardInput;

	// Backward input threshold to exit cover (negative value)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Cover Settings")
	float ExitCoverBackwardThreshold;

	// Allow aiming only at edges
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Cover Settings")
	bool bOnlyAimAtEdges;

	// Interpolation speed for attaching to cover
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Cover Settings")
	float CoverAttachSpeed;

	// Current cover actor
	UPROPERTY(BlueprintReadOnly, Category = "Cover State")
	TObjectPtr<AActor> CurrentCoverActor;

	// Current cover component
	UPROPERTY(BlueprintReadOnly, Category = "Cover State")
	TObjectPtr<UPrimitiveComponent> CurrentCoverComponent;

	// Cover surface normal
	UPROPERTY(BlueprintReadOnly, Category = "Cover State")
	FVector CoverNormal;

	// Cover attachment point
	UPROPERTY(BlueprintReadOnly, Category = "Cover State")
	FVector CoverAttachPoint;

	// Is character currently at an edge
	UPROPERTY(BlueprintReadOnly, Category = "Cover State")
	bool bIsAtEdge;

	// Is the edge on the right side
	UPROPERTY(BlueprintReadOnly, Category = "Cover State")
	bool bIsRightEdge;

private:

	// Timer handle for cover updates
	FTimerHandle CoverUpdateTimerHandle;
	
	// Timer handle for auto-activation checks
	FTimerHandle AutoActivationCheckTimerHandle;
	
	// Cached cover hit from last successful detection (used to avoid re-checking in ActivateAbility)
	mutable FHitResult CachedCoverHit;
	mutable bool bHasCachedCoverHit;
	
	// Check if ability should auto-activate
	void CheckAutoActivation();
};

