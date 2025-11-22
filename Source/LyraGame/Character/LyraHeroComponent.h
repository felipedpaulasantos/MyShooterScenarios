// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/PawnComponent.h"

#include "GameplayAbilitySpecHandle.h"
#include "LyraHeroComponent.generated.h"

namespace EEndPlayReason { enum Type : int; }

class UGameFrameworkComponentManager;
class UInputComponent;
class ULyraCameraMode;
class ULyraInputConfig;
class UObject;
struct FActorInitStateChangedParams;
struct FFrame;
struct FGameplayTag;
struct FInputActionValue;

/**
 * Component that sets up input and camera handling for player controlled pawns (or bots that simulate players).
 * This depends on a PawnExtensionComponent to coordinate initialization.
 */
UCLASS(Blueprintable, Meta=(BlueprintSpawnableComponent))
class LYRAGAME_API ULyraHeroComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:

	ULyraHeroComponent(const FObjectInitializer& ObjectInitializer);

	/** Returns the hero component if one exists on the specified actor. */
	UFUNCTION(BlueprintPure, Category = "Lyra|Hero")
	static ULyraHeroComponent* FindHeroComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<ULyraHeroComponent>() : nullptr); }

	/** Overrides the camera from an active gameplay ability */
	UFUNCTION(BlueprintCallable, Category = "Lyra|Hero")
	void SetAbilityCameraMode(TSubclassOf<ULyraCameraMode> CameraMode, const FGameplayAbilitySpecHandle& OwningSpecHandle);

	/** Clears the camera override if it is set */
	UFUNCTION(BlueprintCallable, Category = "Lyra|Hero")
	void ClearAbilityCameraMode(const FGameplayAbilitySpecHandle& OwningSpecHandle);

	/** Adds mode-specific input config */
	UFUNCTION(BlueprintCallable, Category = "Lyra|Hero|Input")
	void AddAdditionalInputConfig(const ULyraInputConfig* InputConfig);

	/** Removes a mode-specific input config if it has been added */
	UFUNCTION(BlueprintCallable, Category = "Lyra|Hero|Input")
	void RemoveAdditionalInputConfig(const ULyraInputConfig* InputConfig);

	/** True if this is controlled by a real player and has progressed far enough in initialization where additional input bindings can be added */
	UFUNCTION(BlueprintPure, Category = "Lyra|Hero")
	bool IsReadyToBindInputs() const;
	
	/** The name of the extension event sent via UGameFrameworkComponentManager when ability inputs are ready to bind */
	static const FName NAME_BindInputsNow;

	/** The name of this component-implemented feature */
	static const FName NAME_ActorFeatureName;

	/** Blueprint accessors for static names */
	UFUNCTION(BlueprintPure, Category = "Lyra|Hero")
	static FName GetBindInputsNowEventName() { return NAME_BindInputsNow; }

	UFUNCTION(BlueprintPure, Category = "Lyra|Hero")
	static FName GetActorFeatureName() { return NAME_ActorFeatureName; }

	//~ Begin IGameFrameworkInitStateInterface interface
	virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	virtual void CheckDefaultInitialization() override;
	//~ End IGameFrameworkInitStateInterface interface

protected:

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Initialize player input bindings for this hero */
	UFUNCTION(BlueprintCallable, Category = "Lyra|Hero|Input", meta=(BlueprintProtected="true"))
	virtual void InitializePlayerInput(UInputComponent* PlayerInputComponent);

	/** Blueprint hook called after the default player input bindings for this hero have been initialized. */
	UFUNCTION(BlueprintNativeEvent, Category = "Lyra|Hero|Input", meta=(BlueprintProtected="true"))
	void OnInitializePlayerInput(UInputComponent* PlayerInputComponent);

	/** Input: ability tag pressed */
	UFUNCTION(BlueprintCallable, Category = "Lyra|Hero|Input", meta=(BlueprintProtected="true"))
	void Input_AbilityInputTagPressed(FGameplayTag InputTag);
	/** Input: ability tag released */
	UFUNCTION(BlueprintCallable, Category = "Lyra|Hero|Input", meta=(BlueprintProtected="true"))
	void Input_AbilityInputTagReleased(FGameplayTag InputTag);

	UFUNCTION(BlueprintCallable, Category = "Lyra|Hero|Input", meta=(BlueprintProtected="true"))
	void Input_Move(const FInputActionValue& InputActionValue);
	UFUNCTION(BlueprintCallable, Category = "Lyra|Hero|Input", meta=(BlueprintProtected="true"))
	void Input_LookMouse(const FInputActionValue& InputActionValue);
	UFUNCTION(BlueprintCallable, Category = "Lyra|Hero|Input", meta=(BlueprintProtected="true"))
	void Input_LookStick(const FInputActionValue& InputActionValue);
	UFUNCTION(BlueprintCallable, Category = "Lyra|Hero|Input", meta=(BlueprintProtected="true"))
	void Input_Crouch(const FInputActionValue& InputActionValue);
	UFUNCTION(BlueprintCallable, Category = "Lyra|Hero|Input", meta=(BlueprintProtected="true"))
	void Input_AutoRun(const FInputActionValue& InputActionValue);

	/** Determine which camera mode should be active */
	UFUNCTION(BlueprintPure, Category = "Lyra|Hero|Camera", meta=(BlueprintProtected="true"))
	TSubclassOf<ULyraCameraMode> DetermineCameraMode() const;

public:
	// Optional acceleration for right-stick camera look. Disabled by default to preserve behavior.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Hero|Camera", meta=(DisplayName="Enable Right Stick Look Acceleration"))
	bool bLookStickAccelerationEnabled = false;

	/** Reinitialize input bindings for this hero using the current pawn InputComponent, if available. */
	UFUNCTION(BlueprintCallable, Category = "Lyra|Hero|Input")
	void ReinitializePlayerInput();

	// Time (in seconds) to reach full look speed after the stick is engaged beyond the reset threshold.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Hero|Camera", meta=(ClampMin="0.0"))
	float LookStickAccelTimeToMax = 0.25f;

	// Starting speed multiplier when the stick is first engaged. 0.0..1.0 (1.0 = immediate full speed).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Hero|Camera", meta=(ClampMin="0.0", ClampMax="1.0"))
	float LookStickAccelStartScale = 0.35f;

	// Stick magnitude at which the acceleration resets (0..1). Helps avoid small jitter.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Hero|Camera", meta=(ClampMin="0.0", ClampMax="1.0"))
	float LookStickResetThreshold = 0.10f;

	UFUNCTION(BlueprintCallable, Category = "Lyra|Hero|Camera")
	void SetLookStickAccelerationEnabled(bool bEnabled) { bLookStickAccelerationEnabled = bEnabled; }

	UFUNCTION(BlueprintPure, Category = "Lyra|Hero|Camera")
	bool GetLookStickAccelerationEnabled() const { return bLookStickAccelerationEnabled; }

	UFUNCTION(BlueprintCallable, Category = "Lyra|Hero|Camera")
	void ConfigureLookStickAcceleration(float TimeToMax, float StartScale, float ResetThreshold)
	{
		LookStickAccelTimeToMax = FMath::Max(0.f, TimeToMax);
		LookStickAccelStartScale = FMath::Clamp(StartScale, 0.f, 1.f);
		LookStickResetThreshold = FMath::Clamp(ResetThreshold, 0.f, 1.f);
	}

	UFUNCTION(BlueprintPure, Category = "Lyra|Hero|Camera")
	void GetLookStickAcceleration(float& TimeToMax, float& StartScale, float& ResetThreshold) const
	{
		TimeToMax = LookStickAccelTimeToMax;
		StartScale = LookStickAccelStartScale;
		ResetThreshold = LookStickResetThreshold;
	}

protected:
	// DEPRECATED: DefaultInputConfigs removed for UE 5.5
	// The old PlayerMappableInputConfig system has been removed in UE 5.5.
	// Input configs are now loaded through the PawnData's InputConfig directly.
	// Use GameFeatureAction_AddInputConfig for adding input configs at runtime.
	
	/** Camera mode set by an ability. */
	UPROPERTY(BlueprintReadOnly, Category = "Lyra|Hero")
	TSubclassOf<ULyraCameraMode> AbilityCameraMode;

	/** Spec handle for the last ability to set a camera mode. */
	UPROPERTY(BlueprintReadOnly, Category = "Lyra|Hero")
	FGameplayAbilitySpecHandle AbilityCameraModeOwningSpecHandle;

	/** True when player input bindings have been applied, will never be true for non - players */
	UPROPERTY(BlueprintReadOnly, Category = "Lyra|Hero")
	bool bReadyToBindInputs;

	// Internal state for look acceleration (not editable at design time).
	UPROPERTY(Transient)
	float LookStickAccelAlpha = 0.0f;
	UPROPERTY(Transient)
	float LookStickPrevMagnitude = 0.0f;
};
