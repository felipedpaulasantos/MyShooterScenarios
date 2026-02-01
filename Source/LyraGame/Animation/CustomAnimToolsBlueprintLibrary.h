// Copyright (c) 2026

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CustomAnimToolsBlueprintLibrary.generated.h"

/**
 * Blueprint-friendly, thread-safe animation math utilities.
 *
 * Note: Functions in this library must remain side-effect free and must not touch UObjects/World state,
 * so they can be safely executed on the animation worker thread (e.g. inside AnimGraph evaluation).
 */
UCLASS()
class LYRAGAME_API UCustomAnimToolsBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Computes an animation play rate based on current movement speed.
	 *
	 * Mapping rule (piecewise linear):
	 * - If Speed <= ReferenceSpeedForMinRate => returns MinPlayRate
	 * - If Speed >= MaxSpeedForMaxRate      => returns MaxPlayRate
	 * - Otherwise interpolates linearly between them.
	 *
	 * Example: MaxSpeedForMaxRate=600, ReferenceSpeedForMinRate=300, MinPlayRate=0.5, MaxPlayRate=1.0
	 * => Speed<=300 => 0.5, Speed=600 => 1.0.
	 */
	UFUNCTION(BlueprintPure, Category="Lyra|Animation", meta=(BlueprintThreadSafe, DisplayName="Update Anim Rate On Movement Speed", Keywords="Lyra Anim PlayRate RateScale MovementSpeed", AdvancedDisplay="MinPlayRate,MaxPlayRate", ClampMin="0.0"))
	static float UpdateAnimRateOnMovementSpeed(
		float MovementSpeed,
		float ReferenceSpeedForMinRate = 300.f,
		float MaxSpeedForMaxRate = 600.f,
		float MinPlayRate = 0.5f,
		float MaxPlayRate = 1.0f);

	/**
	 * Alias com naming mais explícito (função pura): retorna o PlayRate para um Sequence Player baseado na rampa de velocidade.
	 * Mantém a mesma regra e defaults de UpdateAnimRateOnMovementSpeed.
	 */
	UFUNCTION(BlueprintPure, Category="Lyra|Animation", meta=(BlueprintThreadSafe, DisplayName="Compute Anim Play Rate From Movement Speed", Keywords="Lyra Anim PlayRate RateScale MovementSpeed", AdvancedDisplay="MinPlayRate,MaxPlayRate", ClampMin="0.0"))
	static float ComputeAnimPlayRateFromMovementSpeed(
		float MovementSpeed,
		float ReferenceSpeedForMinRate = 300.f,
		float MaxSpeedForMaxRate = 600.f,
		float MinPlayRate = 0.5f,
		float MaxPlayRate = 1.0f);

	/**
	 * Retorna o Alpha (0..1) correspondente à posição do MovementSpeed dentro do intervalo [ReferenceSpeedForMinRate .. MaxSpeedForMaxRate].
	 *
	 * Regras:
	 * - Speed <= Reference => 0
	 * - Speed >= Max       => 1
	 * - Interpolação linear no meio.
	 */
	UFUNCTION(BlueprintPure, Category="Lyra|Animation", meta=(BlueprintThreadSafe, DisplayName="Normalize Movement Speed For Anim Rate", Keywords="Lyra Anim Normalize Speed Alpha MovementSpeed", ClampMin="0.0"))
	static float NormalizeMovementSpeedForAnimRate(
		float MovementSpeed,
		float ReferenceSpeedForMinRate = 300.f,
		float MaxSpeedForMaxRate = 600.f);
};
