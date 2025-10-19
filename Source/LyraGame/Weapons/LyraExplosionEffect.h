// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraSystem.h"
#include "Camera/CameraShakeBase.h"
#include "Curves/CurveFloat.h"
#include "Sound/SoundBase.h"
#include "Materials/MaterialParameterCollection.h"
#include "LyraExplosionEffect.generated.h"

// Forward declarations
class UNiagaraSystem;
class UCameraShakeBase;
class UCurveFloat;
class USoundBase;
class UMaterialParameterCollection;

/**
 * Efeito de explosão com VFX, camera shake, bloom e outros efeitos pós-processamento
 * Todos os parâmetros são expostos para Blueprints para fácil customização
 */
UCLASS(Blueprintable, BlueprintType)
class LYRAGAME_API ALyraExplosionEffect : public AActor
{
	GENERATED_BODY()
	
public:	
	ALyraExplosionEffect();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	//~=============================================================================
	// Camera Shake Configuration
	//~=============================================================================
	
	/** Classe de Camera Shake a ser aplicada quando a explosão ocorrer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Camera Shake")
	TSubclassOf<UCameraShakeBase> ExplosionCameraShake;

	/** Intensidade máxima do camera shake (multiplicador) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Camera Shake", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float MaxCameraShakeIntensity = 1.5f;

	/** Distância máxima para aplicar o camera shake */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Camera Shake", meta = (ClampMin = "0.0"))
	float CameraShakeMaxDistance = 3000.0f;

	/** Curva que define como a intensidade do shake diminui com a distância (0 = perto, 1 = longe) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Camera Shake")
	TObjectPtr<UCurveFloat> CameraShakeDistanceCurve;

	//~=============================================================================
	// Visual Effects (Niagara)
	//~=============================================================================
	
	/** Sistema de partículas Niagara da explosão principal */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Visual Effects")
	TObjectPtr<UNiagaraSystem> ExplosionParticles;

	/** Sistema de partículas secundário (fumaça, destroços, etc) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Visual Effects")
	TObjectPtr<UNiagaraSystem> SecondaryParticles;

	/** Escala do sistema de partículas */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Visual Effects", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float ParticleScale = 1.0f;

	/** Delay antes de spawnar partículas secundárias (em segundos) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Visual Effects", meta = (ClampMin = "0.0"))
	float SecondaryParticlesDelay = 0.1f;

	//~=============================================================================
	// Post Process Effects
	//~=============================================================================
	
	/** Material Parameter Collection para controlar efeitos de pós-processamento */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Post Process")
	TObjectPtr<UMaterialParameterCollection> PostProcessMPC;

	/** Nome do parâmetro de Bloom no MPC */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Post Process")
	FName BloomParameterName = "ExplosionBloomIntensity";

	/** Intensidade máxima do Bloom */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Post Process", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float MaxBloomIntensity = 3.0f;

	/** Duração do efeito de Bloom (em segundos) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Post Process", meta = (ClampMin = "0.0"))
	float BloomDuration = 0.5f;

	/** Curva de animação do Bloom (0 = início, 1 = fim) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Post Process")
	TObjectPtr<UCurveFloat> BloomCurve;

	/** Nome do parâmetro de Chromatic Aberration no MPC */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Post Process")
	FName ChromaticAberrationParameterName = "ExplosionChromaticAberration";

	/** Intensidade máxima da Chromatic Aberration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Post Process", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float MaxChromaticAberration = 0.8f;

	/** Duração do efeito de Chromatic Aberration (em segundos) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Post Process", meta = (ClampMin = "0.0"))
	float ChromaticAberrationDuration = 0.3f;

	/** Nome do parâmetro de Vignette no MPC */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Post Process")
	FName VignetteParameterName = "ExplosionVignetteIntensity";

	/** Intensidade máxima do Vignette */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Post Process", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxVignetteIntensity = 0.7f;

	/** Duração do efeito de Vignette (em segundos) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Post Process", meta = (ClampMin = "0.0"))
	float VignetteDuration = 1.0f;

	/** Nome do parâmetro de Desaturation no MPC */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Post Process")
	FName DesaturationParameterName = "ExplosionDesaturation";

	/** Quantidade máxima de dessaturação (0 = colorido, 1 = preto e branco) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Post Process", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxDesaturation = 0.5f;

	/** Duração do efeito de Desaturation (em segundos) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Post Process", meta = (ClampMin = "0.0"))
	float DesaturationDuration = 2.0f;

	//~=============================================================================
	// Audio
	//~=============================================================================
	
	/** Som da explosão */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Audio")
	TObjectPtr<USoundBase> ExplosionSound;

	/** Volume do som da explosão */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ExplosionSoundVolume = 1.0f;

	/** Som de debris/destroços */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Audio")
	TObjectPtr<USoundBase> DebrisSound;

	/** Delay antes de tocar o som de debris (em segundos) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Audio", meta = (ClampMin = "0.0"))
	float DebrisSoundDelay = 0.2f;

	//~=============================================================================
	// Physics & Damage
	//~=============================================================================
	
	/** Distância máxima para aplicar efeitos da explosão */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Physics")
	float MaxEffectDistance = 2000.0f;

	/** Força da explosão (para objetos físicos) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Physics", meta = (ClampMin = "0.0"))
	float ExplosionForce = 500000.0f;

	/** Raio da força radial */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Physics", meta = (ClampMin = "0.0"))
	float ExplosionRadius = 1000.0f;

	/** Aplicar falloff linear na força */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Physics")
	bool bApplyForceFalloff = true;

	//~=============================================================================
	// General Settings
	//~=============================================================================
	
	/** Se true, o efeito afeta apenas o jogador local */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|General")
	bool bLocalPlayerOnly = false;

	/** Se true, o actor é destruído automaticamente após os efeitos terminarem */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|General")
	bool bAutoDestroy = true;

	/** Tempo até destruir o actor (em segundos) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|General", meta = (ClampMin = "0.0", EditCondition = "bAutoDestroy"))
	float AutoDestroyDelay = 5.0f;

	/** Se true, mostra debug visuals */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|Debug")
	bool bShowDebug = false;

	//~=============================================================================
	// Public Methods
	//~=============================================================================
	
	/** 
	 * Dispara a explosão na localização especificada
	 * @param Location - Localização da explosão
	 */
	UFUNCTION(BlueprintCallable, Category = "Explosion")
	void TriggerExplosion(FVector Location);

	/** 
	 * Dispara a explosão na localização atual do actor
	 */
	UFUNCTION(BlueprintCallable, Category = "Explosion")
	void TriggerExplosionAtActorLocation();

	/** 
	 * Aplica camera shake ao jogador com base na distância
	 * @param PlayerController - Controller do jogador
	 * @param Distance - Distância entre o jogador e a explosão
	 */
	UFUNCTION(BlueprintCallable, Category = "Explosion")
	void ApplyCameraShake(APlayerController* PlayerController, float Distance);

	/** 
	 * Aplica efeitos de pós-processamento baseado na distância
	 * @param Distance - Distância entre o jogador e a explosão
	 */
	UFUNCTION(BlueprintCallable, Category = "Explosion")
	void ApplyPostProcessEffects(float Distance);

	/** 
	 * Spawna o sistema de partículas da explosão
	 * @param Location - Localização onde spawnar as partículas
	 */
	UFUNCTION(BlueprintCallable, Category = "Explosion")
	void SpawnExplosionParticles(FVector Location);

	/** 
	 * Toca os sons da explosão
	 * @param Location - Localização da explosão
	 */
	UFUNCTION(BlueprintCallable, Category = "Explosion")
	void PlayExplosionSounds(FVector Location);

	/** 
	 * Aplica força radial aos objetos físicos próximos
	 * @param Location - Centro da explosão
	 */
	UFUNCTION(BlueprintCallable, Category = "Explosion")
	void ApplyRadialForce(FVector Location);

	/** 
	 * Calcula a intensidade do efeito baseado na distância (0 = longe, 1 = perto)
	 * @param Distance - Distância entre o jogador e a explosão
	 * @return Intensidade normalizada (0.0 a 1.0)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Explosion")
	float CalculateEffectIntensity(float Distance) const;

	/** 
	 * Para todos os efeitos de pós-processamento imediatamente
	 */
	UFUNCTION(BlueprintCallable, Category = "Explosion")
	void StopAllPostProcessEffects();

	//~=============================================================================
	// Blueprint Events
	//~=============================================================================
	
	/** Evento chamado quando a explosão é disparada */
	UFUNCTION(BlueprintImplementableEvent, Category = "Explosion", meta = (DisplayName = "On Explosion Triggered"))
	void BP_OnExplosionTriggered(FVector Location);

	/** Evento chamado quando os efeitos de pós-processamento são aplicados */
	UFUNCTION(BlueprintImplementableEvent, Category = "Explosion", meta = (DisplayName = "On Post Process Applied"))
	void BP_OnPostProcessApplied(float Intensity);

	/** Evento chamado quando o camera shake é aplicado */
	UFUNCTION(BlueprintImplementableEvent, Category = "Explosion", meta = (DisplayName = "On Camera Shake Applied"))
	void BP_OnCameraShakeApplied(float Intensity);

private:
	/** Tempo atual do efeito de bloom */
	float CurrentBloomTime;

	/** Tempo atual do efeito de chromatic aberration */
	float CurrentChromaticAberrationTime;

	/** Tempo atual do efeito de vignette */
	float CurrentVignetteTime;

	/** Tempo atual do efeito de desaturation */
	float CurrentDesaturationTime;

	/** Se true, os efeitos de pós-processamento estão ativos */
	bool bPostProcessEffectsActive;

	/** Atualiza os efeitos de pós-processamento ao longo do tempo */
	void UpdatePostProcessEffects(float DeltaTime);

	/** Reseta todos os timers de pós-processamento */
	void ResetPostProcessTimers();

	/** Desenha debug visual */
	void DrawDebugVisuals(FVector Location, float Distance, float Intensity);

	/** Timer handle para destruição automática */
	FTimerHandle AutoDestroyTimerHandle;

	/** Função chamada pelo timer de auto-destruição */
	void HandleAutoDestroy();
};
