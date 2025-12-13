// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraHeroComponent.h"
#include "Components/GameFrameworkComponentDelegates.h"
#include "Logging/MessageLog.h"
#include "LyraLogChannels.h"
#include "EnhancedInputSubsystems.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "Player/LyraPlayerController.h"
#include "Player/LyraPlayerState.h"
#include "Player/LyraLocalPlayer.h"
#include "Character/LyraPawnExtensionComponent.h"
#include "Character/LyraPawnData.h"
#include "Character/LyraCharacter.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Input/LyraInputConfig.h"
#include "Input/LyraInputComponent.h"
#include "Camera/LyraCameraComponent.h"
#include "LyraGameplayTags.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Camera/LyraCameraMode.h"
#include "InputMappingContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraHeroComponent)

#if WITH_EDITOR
#include "Misc/UObjectToken.h"
#endif	// WITH_EDITOR

namespace LyraHero
{
	static const float LookYawRate = 300.0f;
	static const float LookPitchRate = 165.0f;
};

const FName ULyraHeroComponent::NAME_BindInputsNow("BindInputsNow");
const FName ULyraHeroComponent::NAME_ActorFeatureName("Hero");

ULyraHeroComponent::ULyraHeroComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilityCameraMode = nullptr;
	bReadyToBindInputs = false;

	// Initialize look stick temporal smoothing state.
	LookStickSmoothedValue = FVector2D::ZeroVector;
}

void ULyraHeroComponent::OnRegister()
{
	Super::OnRegister();

	if (!GetPawn<APawn>())
	{
		UE_LOG(LogLyra, Error, TEXT("[ULyraHeroComponent::OnRegister] This component has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint."));

#if WITH_EDITOR
		if (GIsEditor)
		{
			static const FText Message = NSLOCTEXT("LyraHeroComponent", "NotOnPawnError", "has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint. This will cause a crash if you PIE!");
			static const FName HeroMessageLogName = TEXT("LyraHeroComponent");
			
			FMessageLog(HeroMessageLogName).Error()
				->AddToken(FUObjectToken::Create(this, FText::FromString(GetNameSafe(this))))
				->AddToken(FTextToken::Create(Message));
				
			FMessageLog(HeroMessageLogName).Open();
		}
#endif
	}
	else
	{
		// Register with the init state system early, this will only work if this is a game world
		RegisterInitStateFeature();
	}
}

bool ULyraHeroComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);

	APawn* Pawn = GetPawn<APawn>();

	if (!CurrentState.IsValid() && DesiredState == LyraGameplayTags::InitState_Spawned)
	{
		// As long as we have a real pawn, let us transition
		if (Pawn)
		{
			return true;
		}
	}
	else if (CurrentState == LyraGameplayTags::InitState_Spawned && DesiredState == LyraGameplayTags::InitState_DataAvailable)
	{
		// The player state is required.
		if (!GetPlayerState<ALyraPlayerState>())
		{
			return false;
		}

		// If we're authority or autonomous, we need to wait for a controller with registered ownership of the player state.
		if (Pawn->GetLocalRole() != ROLE_SimulatedProxy)
		{
			AController* Controller = GetController<AController>();

			const bool bHasControllerPairedWithPS = (Controller != nullptr) && \
				(Controller->PlayerState != nullptr) && \
				(Controller->PlayerState->GetOwner() == Controller);

			if (!bHasControllerPairedWithPS)
			{
				return false;
			}
		}

		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();
		const bool bIsBot = Pawn->IsBotControlled();

		if (bIsLocallyControlled && !bIsBot)
		{
			ALyraPlayerController* LyraPC = GetController<ALyraPlayerController>();

			// The input component and local player is required when locally controlled.
			if (!Pawn->InputComponent || !LyraPC || !LyraPC->GetLocalPlayer())
			{
				return false;
			}
		}

		return true;
	}
	else if (CurrentState == LyraGameplayTags::InitState_DataAvailable && DesiredState == LyraGameplayTags::InitState_DataInitialized)
	{
		// Wait for player state and extension component
		ALyraPlayerState* LyraPS = GetPlayerState<ALyraPlayerState>();

		return LyraPS && Manager->HasFeatureReachedInitState(Pawn, ULyraPawnExtensionComponent::NAME_ActorFeatureName, LyraGameplayTags::InitState_DataInitialized);
	}
	else if (CurrentState == LyraGameplayTags::InitState_DataInitialized && DesiredState == LyraGameplayTags::InitState_GameplayReady)
	{
		// TODO add ability initialization checks?
		return true;
	}

	return false;
}

void ULyraHeroComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (CurrentState == LyraGameplayTags::InitState_DataAvailable && DesiredState == LyraGameplayTags::InitState_DataInitialized)
	{
		APawn* Pawn = GetPawn<APawn>();
		ALyraPlayerState* LyraPS = GetPlayerState<ALyraPlayerState>();
		if (!ensure(Pawn && LyraPS))
		{
			return;
		}

		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();
		const ULyraPawnData* PawnData = nullptr;

		if (ULyraPawnExtensionComponent* PawnExtComp = ULyraPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			PawnData = PawnExtComp->GetPawnData<ULyraPawnData>();

			// The player state holds the persistent data for this player (state that persists across deaths and multiple pawns).
			// The ability system component and attribute sets live on the player state.
			PawnExtComp->InitializeAbilitySystem(LyraPS->GetLyraAbilitySystemComponent(), LyraPS);
		}

		if (ALyraPlayerController* LyraPC = GetController<ALyraPlayerController>())
		{
			if (Pawn->InputComponent != nullptr)
			{
				InitializePlayerInput(Pawn->InputComponent);
			}
		}

		if (bIsLocallyControlled && PawnData)
		{
			if (ULyraCameraComponent* CameraComponent = ULyraCameraComponent::FindCameraComponent(Pawn))
			{
				CameraComponent->DetermineCameraModeDelegate.BindUObject(this, &ThisClass::DetermineCameraMode);
			}
		}
	}
}

void ULyraHeroComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	if (Params.FeatureName == ULyraPawnExtensionComponent::NAME_ActorFeatureName)
	{
		if (Params.FeatureState == LyraGameplayTags::InitState_DataInitialized)
		{
			// If the extension component says all all other components are initialized, try to progress to next state
			CheckDefaultInitialization();
		}
	}
}

void ULyraHeroComponent::CheckDefaultInitialization()
{
	static const TArray<FGameplayTag> StateChain = { LyraGameplayTags::InitState_Spawned, LyraGameplayTags::InitState_DataAvailable, LyraGameplayTags::InitState_DataInitialized, LyraGameplayTags::InitState_GameplayReady };

	// This will try to progress from spawned (which is only set in BeginPlay) through the data initialization stages until it gets to gameplay ready
	ContinueInitStateChain(StateChain);
}

void ULyraHeroComponent::BeginPlay()
{
	Super::BeginPlay();

	// Listen for when the pawn extension component changes init state
	BindOnActorInitStateChanged(ULyraPawnExtensionComponent::NAME_ActorFeatureName, FGameplayTag(), false);

	// Notifies that we are done spawning, then try the rest of initialization
	ensure(TryToChangeInitState(LyraGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void ULyraHeroComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();

	Super::EndPlay(EndPlayReason);
}

void ULyraHeroComponent::InitializePlayerInput(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		UE_LOG(LogLyra, Warning, TEXT("[LyraHeroComponent::InitializePlayerInput] No Pawn owner"));
		return;
	}

	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const ULyraLocalPlayer* LP = Cast<ULyraLocalPlayer>(PC->GetLocalPlayer());
	check(LP);
	
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

	Subsystem->ClearAllMappings();

	if (const ULyraPawnExtensionComponent* PawnExtComp = ULyraPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const ULyraPawnData* PawnData = PawnExtComp->GetPawnData<ULyraPawnData>())
		{
			if (const ULyraInputConfig* InputConfig = PawnData->InputConfig)
			{
				// UE 5.5: The old PlayerMappableInputConfig system has been removed
				// Input configs are now loaded through the PawnData's InputConfig directly
				// The DefaultInputConfigs array is deprecated and should not be used
				
				// The Lyra Input Component has some additional functions to map Gameplay Tags to an Input Action.
				// If you want this functionality but still want to change your input component class, make it a subclass
				// of the ULyraInputComponent or modify this component accordingly.
				ULyraInputComponent* LyraIC = Cast<ULyraInputComponent>(PlayerInputComponent);
				if (ensureMsgf(LyraIC, TEXT("Unexpected Input Component class! The Gameplay Abilities will not be bound to their inputs. Change the input component to ULyraInputComponent or a subclass of it.")))
				{
					// Clear any previous bindings/mappings from this input component so we don't end up
					// with duplicated ability bindings or stale mappings on repossess/reuse flows.
					LyraIC->ClearActionBindings();
					LyraIC->AxisBindings.Reset();

					// Add the key mappings that may have been set by the player
					LyraIC->AddInputMappings(InputConfig, Subsystem);

					// This is where we actually bind and input action to a gameplay tag, which means that Gameplay Ability Blueprints will
					// be triggered directly by these input actions Triggered events. 
					TArray<uint32> BindHandles;
					LyraIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);

					LyraIC->BindNativeAction(InputConfig, LyraGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &ThisClass::Input_Move, /*bLogIfNotFound=*/ false);
					LyraIC->BindNativeAction(InputConfig, LyraGameplayTags::InputTag_Look_Mouse, ETriggerEvent::Triggered, this, &ThisClass::Input_LookMouse, /*bLogIfNotFound=*/ false);
					LyraIC->BindNativeAction(InputConfig, LyraGameplayTags::InputTag_Look_Stick, ETriggerEvent::Triggered, this, &ThisClass::Input_LookStick, /*bLogIfNotFound=*/ false);
					LyraIC->BindNativeAction(InputConfig, LyraGameplayTags::InputTag_Crouch, ETriggerEvent::Triggered, this, &ThisClass::Input_Crouch, /*bLogIfNotFound=*/ false);
					LyraIC->BindNativeAction(InputConfig, LyraGameplayTags::InputTag_AutoRun, ETriggerEvent::Triggered, this, &ThisClass::Input_AutoRun, /*bLogIfNotFound=*/ false);
				}
			}
			else
			{
				UE_LOG(LogLyra, Warning, TEXT("[LyraHeroComponent::InitializePlayerInput] PawnData has NO InputConfig"));
			}
		}
		else
		{
			UE_LOG(LogLyra, Warning, TEXT("[LyraHeroComponent::InitializePlayerInput] PawnExtComp has NO PawnData"));
		}
	}
	else
	{
		UE_LOG(LogLyra, Warning, TEXT("[LyraHeroComponent::InitializePlayerInput] No PawnExtensionComponent found for Pawn=%s"), *GetNameSafe(Pawn));
	}

	// Allow Blueprint subclasses to extend input initialization after the default Lyra bindings are in place.
	OnInitializePlayerInput(PlayerInputComponent);

	// Mark that inputs have been bound; in reuse/repossess flows this may be called more than once,
	// so avoid asserting on repeated initialization.
	bReadyToBindInputs = true;
 
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APlayerController*>(PC), NAME_BindInputsNow);
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APawn*>(Pawn), NAME_BindInputsNow);
}

void ULyraHeroComponent::ReinitializePlayerInput()
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		UE_LOG(LogLyra, Warning, TEXT("[LyraHeroComponent::ReinitializePlayerInput] No Pawn owner"));
		return;
	}
	
	if (UInputComponent* PlayerInputComponent = Pawn->InputComponent)
	{
		InitializePlayerInput(PlayerInputComponent);
	}
}

void ULyraHeroComponent::OnInitializePlayerInput_Implementation(UInputComponent* PlayerInputComponent)
{
	// Default implementation does nothing; override in C++ subclasses or Blueprints to extend input bindings.
}

void ULyraHeroComponent::AddAdditionalInputConfig(const ULyraInputConfig* InputConfig)
{
	TArray<uint32> BindHandles;

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}
	
	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const ULocalPlayer* LP = PC->GetLocalPlayer();
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

	if (const ULyraPawnExtensionComponent* PawnExtComp = ULyraPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		ULyraInputComponent* LyraIC = Pawn->FindComponentByClass<ULyraInputComponent>();
		if (ensureMsgf(LyraIC, TEXT("Unexpected Input Component class! The Gameplay Abilities will not be bound to their inputs. Change the input component to ULyraInputComponent or a subclass of it.")))
		{
			LyraIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);
		}
	}
}

void ULyraHeroComponent::RemoveAdditionalInputConfig(const ULyraInputConfig* InputConfig)
{
	//@TODO: Implement me!
}

bool ULyraHeroComponent::IsReadyToBindInputs() const
{
	return bReadyToBindInputs;
}

void ULyraHeroComponent::Input_AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (const APawn* Pawn = GetPawn<APawn>())
	{
		if (const ULyraPawnExtensionComponent* PawnExtComp = ULyraPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			if (ULyraAbilitySystemComponent* LyraASC = PawnExtComp->GetLyraAbilitySystemComponent())
			{
				// For debugging: count abilities that are bound to this input tag.
				const TArray<FGameplayAbilitySpec>& Specs = LyraASC->GetActivatableAbilities();
				int32 MatchingCount = 0;
				for (const FGameplayAbilitySpec& Spec : Specs)
				{
					if (Spec.Ability)
					{
						const FGameplayTagContainer& AbilityTags = Spec.GetDynamicSpecSourceTags();
						if (AbilityTags.HasTagExact(InputTag))
						{
							++MatchingCount;
						}
					}
				}
				
				LyraASC->AbilityInputTagPressed(InputTag);
			}
			else
			{
				UE_LOG(LogLyra, Warning,
					TEXT("[LyraHeroComponent::Input_AbilityInputTagPressed] No ASC found for Pawn=%s"),
					*GetNameSafe(Pawn));
			}
		}
	}
}

void ULyraHeroComponent::Input_AbilityInputTagReleased(FGameplayTag InputTag)
{
	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	if (const ULyraPawnExtensionComponent* PawnExtComp = ULyraPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (ULyraAbilitySystemComponent* LyraASC = PawnExtComp->GetLyraAbilitySystemComponent())
		{
			LyraASC->AbilityInputTagReleased(InputTag);
		}
	}
}

void ULyraHeroComponent::Input_Move(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;

	// If the player has attempted to move again then cancel auto running
	if (ALyraPlayerController* LyraController = Cast<ALyraPlayerController>(Controller))
	{
		LyraController->SetIsAutoRunning(false);
	}
	
	if (Controller)
	{
		const FVector2D Value = InputActionValue.Get<FVector2D>();
		const FRotator MovementRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);

		if (Value.X != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::RightVector);
			Pawn->AddMovementInput(MovementDirection, Value.X);
		}

		if (Value.Y != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::ForwardVector);
			Pawn->AddMovementInput(MovementDirection, Value.Y);
		}
	}
}

void ULyraHeroComponent::Input_LookMouse(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();

	if (!Pawn)
	{
		return;
	}
	
	const FVector2D Value = InputActionValue.Get<FVector2D>();

	if (Value.X != 0.0f)
	{
		Pawn->AddControllerYawInput(Value.X);
	}

	if (Value.Y != 0.0f)
	{
		Pawn->AddControllerPitchInput(Value.Y);
	}
}

void ULyraHeroComponent::Input_LookStick(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();

	if (!Pawn)
	{
		return;
	}
	
	FVector2D RawValue = InputActionValue.Get<FVector2D>();

	const UWorld* World = GetWorld();
	check(World);

	FVector2D Value = RawValue;

	// Time + magnitude-based acceleration for gamepad look.
	if (LookStickAccelerationCurve)
	{
		const float Magnitude = RawValue.Size();
		const float DeltaTime = World->GetDeltaSeconds();

		// Deadzone "forte" para considerar o stick realmente parado.
		constexpr float StickDeadZone = 0.10f;

		if (Magnitude < StickDeadZone)
		{
			// Stick voltou para perto do centro: ramp-down suave e reset completo perto de zero.
			LookStickPrevDirection = FVector2D::ZeroVector;

			const float RampDownSpeed = 50.0f; // 1/s, maior = desce mais rápido
			const float PrevMultiplier = LookStickCurrentMultiplier;
			LookStickCurrentMultiplier = FMath::FInterpTo(LookStickCurrentMultiplier, 0.0f, DeltaTime, RampDownSpeed);

			if (LookStickCurrentMultiplier <= 0.02f)
			{
				// Reset completo do estado para evitar acumular valor residual.
				LookStickCurrentMultiplier = 0.0f;
				LookStickTimeSinceEngaged = 0.0f;
				LookStickSmoothedValue = FVector2D::ZeroVector;
				Value = FVector2D::ZeroVector;

				UE_LOG(LogLyra, VeryVerbose,
					TEXT("[LyraHeroComponent::Input_LookStick] Final reset: Mag=%.4f PrevMult=%.4f NewMult=%.4f"),
					Magnitude, PrevMultiplier, LookStickCurrentMultiplier);
			}
			else
			{
				Value = RawValue * LookStickCurrentMultiplier;

				UE_LOG(LogLyra, VeryVerbose,
					TEXT("[LyraHeroComponent::Input_LookStick] Deadzone ramp-down: Mag=%.4f PrevMult=%.4f NewMult=%.4f"),
					Magnitude, PrevMultiplier, LookStickCurrentMultiplier);
			}
		}
		else
		{
			// Stick ativo: o tempo da curva continua subindo (independente da direção)
			// para evitar micro-trancos quando o jogador gira o stick em círculo.
			const FVector2D Direction = RawValue / Magnitude;
			LookStickPrevDirection = Direction;

			LookStickTimeSinceEngaged += DeltaTime;

			// Curve X = tempo (s), Y = multiplicador alvo base.
			const float TimeCurveValue = FMath::Max(0.0f, LookStickAccelerationCurve->GetFloatValue(LookStickTimeSinceEngaged));

			// Fator baseado na magnitude do stick: quanto mais próximo de 1, mais próximo do valor da curva.
			// - Em Magnitude=1 -> MagnitudeFactor=1 (usa o valor cheio da curva)
			// - Em Magnitude=0.2 -> MagnitudeFactor~0.25 (ramp-up bem mais lento)
			// Ajuste o expoente se quiser mais/menos sensibilidade.
			const float MagnitudeFactor = FMath::Pow(Magnitude, 1.5f);

			const float TargetMultiplier = TimeCurveValue * MagnitudeFactor;

			const float PrevMultiplier = LookStickCurrentMultiplier;
			// Ramp-up base; será efetivamente mais rápido para magnitudes altas
			// porque TargetMultiplier sobe mais rápido.
			const float RampUpSpeed = 2.0f;
			LookStickCurrentMultiplier = FMath::FInterpTo(LookStickCurrentMultiplier, TargetMultiplier, DeltaTime, RampUpSpeed);

			Value = RawValue * LookStickCurrentMultiplier;

			UE_LOG(LogLyra, Warning,
				TEXT("[LyraHeroComponent::Input_LookStick] Active: Mag=%.3f Time=%.3f TimeCurve=%.3f MagFactor=%.3f Target=%.3f PrevMult=%.3f NewMult=%.3f"),
				Magnitude, LookStickTimeSinceEngaged, TimeCurveValue, MagnitudeFactor, TargetMultiplier, PrevMultiplier, LookStickCurrentMultiplier);
		}
	}

	// ------------------------
	// Temporal smoothing layer
	// ------------------------
	// Aplica um filtro exponencial leve para reduzir jitter de pequenas correções
	// sem alterar o comportamento existente quando a força de suavização é zero.
	FVector2D FinalValue = Value;

	const float DeltaTime = World->GetDeltaSeconds();

	if (LookStickSmoothingStrength > 0.0f)
	{
		// Mapear LookStickSmoothingStrength [0,1] para uma "velocidade" de suavização em 1/s.
		// Valores típicos: 0.0 = sem filtro, 0.3 ~ 0.5 = filtro leve.
		const float MinSmoothingSpeed = 10.0f;
		const float MaxExtraSmoothingSpeed = 40.0f;
		const float SmoothingSpeed = MinSmoothingSpeed + (MaxExtraSmoothingSpeed * FMath::Clamp(LookStickSmoothingStrength, 0.0f, 1.0f));

		const float Alpha = 1.0f - FMath::Exp(-SmoothingSpeed * DeltaTime);

		FVector2D Target = Value;

		// Atualiza valor suavizado.
		LookStickSmoothedValue = FMath::Lerp(LookStickSmoothedValue, Target, Alpha);

		// Opcional: limitar velocidade máxima de variação em graus/segundo, se configurado.
		if (LookStickSmoothingMaxLagDegreesPerSec > 0.0f)
		{
			// Converte delta em algo aproximado a deg/s usando os rates atuais.
			FVector2D DeltaStick = LookStickSmoothedValue - FinalValue;
			const float MaxDeltaYawDeg = LookStickSmoothingMaxLagDegreesPerSec * DeltaTime / LyraHero::LookYawRate;
			const float MaxDeltaPitchDeg = LookStickSmoothingMaxLagDegreesPerSec * DeltaTime / LyraHero::LookPitchRate;

			DeltaStick.X = FMath::Clamp(DeltaStick.X, -MaxDeltaYawDeg, MaxDeltaYawDeg);
			DeltaStick.Y = FMath::Clamp(DeltaStick.Y, -MaxDeltaPitchDeg, MaxDeltaPitchDeg);

			LookStickSmoothedValue = FinalValue + DeltaStick;
		}

		FinalValue = LookStickSmoothedValue;
	}

	if (FinalValue.X != 0.0f)
	{
		Pawn->AddControllerYawInput(FinalValue.X * LyraHero::LookYawRate * DeltaTime);
	}

	if (FinalValue.Y != 0.0f)
	{
		Pawn->AddControllerPitchInput(FinalValue.Y * LyraHero::LookPitchRate * DeltaTime);
	}
}

void ULyraHeroComponent::Input_Crouch(const FInputActionValue& InputActionValue)
{
	if (ALyraCharacter* Character = GetPawn<ALyraCharacter>())
	{
		Character->ToggleCrouch();
	}
}

void ULyraHeroComponent::Input_AutoRun(const FInputActionValue& InputActionValue)
{
	if (APawn* Pawn = GetPawn<APawn>())
	{
		if (ALyraPlayerController* Controller = Cast<ALyraPlayerController>(Pawn->GetController()))
		{
			// Toggle auto running
			Controller->SetIsAutoRunning(!Controller->GetIsAutoRunning());
		}	
	}
}

TSubclassOf<ULyraCameraMode> ULyraHeroComponent::DetermineCameraMode() const
{
	if (AbilityCameraMode)
	{
		return AbilityCameraMode;
	}

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return nullptr;
	}

	if (ULyraPawnExtensionComponent* PawnExtComp = ULyraPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const ULyraPawnData* PawnData = PawnExtComp->GetPawnData<ULyraPawnData>())
		{
			return PawnData->DefaultCameraMode;
		}
	}

	return nullptr;
}

void ULyraHeroComponent::SetAbilityCameraMode(TSubclassOf<ULyraCameraMode> CameraMode, const FGameplayAbilitySpecHandle& OwningSpecHandle)
{
	if (CameraMode)
	{
		AbilityCameraMode = CameraMode;
		AbilityCameraModeOwningSpecHandle = OwningSpecHandle;
	}
}

void ULyraHeroComponent::ClearAbilityCameraMode(const FGameplayAbilitySpecHandle& OwningSpecHandle)
{
	if (AbilityCameraModeOwningSpecHandle == OwningSpecHandle)
	{
		AbilityCameraMode = nullptr;
		AbilityCameraModeOwningSpecHandle = FGameplayAbilitySpecHandle();
	}
}
