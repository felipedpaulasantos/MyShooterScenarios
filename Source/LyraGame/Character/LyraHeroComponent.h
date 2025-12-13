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
	// Optional acceleration curve for right-stick camera look. If null, stick input is passed through unmodified.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Hero|Camera", meta=(DisplayName="Gamepad Look Stick Acceleration Curve"))
	class UCurveFloat* LookStickAccelerationCurve = nullptr;

	/** Strength of the temporal smoothing filter applied to the right stick look input (gamepad only).
	 * 0.0 = disabled (original behaviour, no additional temporal smoothing).
	 * 1.0 = strong smoothing (more stable but slower to respond to big changes).
	 *
	 * This is evaluated in C++ when handling `IA_Look_Stick` and can be tweaked in blueprints per-hero.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Hero|Camera")
	float LookStickSmoothingStrength = 0.0f;

	/** Optional maximum rate, in normalized stick units per second, at which the smoothed
	 * look value is allowed to change. 0.0 means "no explicit clamp", only the
	 * exponential smoothing defined by LookStickSmoothingStrength is used.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Hero|Camera")
	float LookStickSmoothingMaxDeltaPerSecond = 0.0f;

	/** Optional clamp for how quickly the temporal smoothing is allowed to lag behind the
	 * raw stick input, expressed in degrees/second of camera rotation. Set to 0 to disable
	 * this clamp and rely purely on the exponential smoothing + ramp curve.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lyra|Hero|Camera")
	float LookStickSmoothingMaxLagDegreesPerSec = 0.0f;

	/** Reinitialize input bindings for this hero using the current pawn InputComponent, if available. */
	UFUNCTION(BlueprintCallable, Category = "Lyra|Hero|Input")
	void ReinitializePlayerInput();

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

	// Time-based state for right-stick look acceleration (gamepad only).
	UPROPERTY(Transient)
	float LookStickTimeSinceEngaged = 0.0f;

	UPROPERTY(Transient)
	FVector2D LookStickPrevDirection = FVector2D::ZeroVector;

	// Cached multiplier used for smooth ramp-down when the stick is released.
	UPROPERTY(Transient)
	float LookStickCurrentMultiplier = 0.0f;

	/** Temporally smoothed right-stick value used by Input_LookStick (after deadzone and acceleration).
	 * Stored in normalized stick units, not in degrees/second.
	 */
	UPROPERTY(Transient)
	FVector2D LookStickSmoothedValue = FVector2D::ZeroVector;
};
