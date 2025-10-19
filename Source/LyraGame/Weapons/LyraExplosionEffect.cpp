// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraExplosionEffect.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "CollisionQueryParams.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/OverlapResult.h"

ALyraExplosionEffect::ALyraExplosionEffect()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// Inicialização dos valores padrão
	CurrentBloomTime = 0.0f;
	CurrentChromaticAberrationTime = 0.0f;
	CurrentVignetteTime = 0.0f;
	CurrentDesaturationTime = 0.0f;
	bPostProcessEffectsActive = false;

	// Root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
}

void ALyraExplosionEffect::BeginPlay()
{
	Super::BeginPlay();
}

void ALyraExplosionEffect::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bPostProcessEffectsActive)
	{
		UpdatePostProcessEffects(DeltaTime);
	}
}

void ALyraExplosionEffect::TriggerExplosion(FVector Location)
{
	// Chama o evento Blueprint
	BP_OnExplosionTriggered(Location);

	// Spawn partículas
	SpawnExplosionParticles(Location);

	// Toca sons
	PlayExplosionSounds(Location);

	// Aplica força radial
	ApplyRadialForce(Location);

	// Encontra todos os jogadores e aplica efeitos baseados na distância
	if (bLocalPlayerOnly)
	{
		// Apenas o jogador local
		APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (PC && PC->GetPawn())
		{
			float Distance = FVector::Dist(Location, PC->GetPawn()->GetActorLocation());
			
			if (Distance <= MaxEffectDistance)
			{
				ApplyCameraShake(PC, Distance);
				ApplyPostProcessEffects(Distance);

				if (bShowDebug)
				{
					float Intensity = CalculateEffectIntensity(Distance);
					DrawDebugVisuals(Location, Distance, Intensity);
				}
			}
		}
	}
	else
	{
		// Todos os jogadores
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			APlayerController* PC = Iterator->Get();
			if (PC && PC->GetPawn())
			{
				float Distance = FVector::Dist(Location, PC->GetPawn()->GetActorLocation());
				
				if (Distance <= MaxEffectDistance)
				{
					ApplyCameraShake(PC, Distance);
					ApplyPostProcessEffects(Distance);

					if (bShowDebug)
					{
						float Intensity = CalculateEffectIntensity(Distance);
						DrawDebugVisuals(Location, Distance, Intensity);
					}
				}
			}
		}
	}

	// Auto-destruição
	if (bAutoDestroy)
	{
		GetWorldTimerManager().SetTimer(AutoDestroyTimerHandle, this, &ALyraExplosionEffect::HandleAutoDestroy, AutoDestroyDelay, false);
	}
}

void ALyraExplosionEffect::TriggerExplosionAtActorLocation()
{
	TriggerExplosion(GetActorLocation());
}

void ALyraExplosionEffect::ApplyCameraShake(APlayerController* PlayerController, float Distance)
{
	if (!PlayerController || !ExplosionCameraShake)
	{
		return;
	}

	// Calcula intensidade baseada na distância
	float NormalizedDistance = FMath::Clamp(Distance / CameraShakeMaxDistance, 0.0f, 1.0f);
	float Intensity = MaxCameraShakeIntensity;

	// Usa curva se disponível
	if (CameraShakeDistanceCurve)
	{
		float CurveValue = CameraShakeDistanceCurve->GetFloatValue(NormalizedDistance);
		Intensity *= CurveValue;
	}
	else
	{
		// Falloff linear padrão
		Intensity *= (1.0f - NormalizedDistance);
	}

	// Aplica o camera shake
	if (Intensity > 0.0f)
	{
		PlayerController->ClientStartCameraShake(ExplosionCameraShake, Intensity);
		
		// Chama evento Blueprint
		BP_OnCameraShakeApplied(Intensity);
	}
}

void ALyraExplosionEffect::ApplyPostProcessEffects(float Distance)
{
	if (!PostProcessMPC)
	{
		return;
	}

	// Calcula intensidade
	float Intensity = CalculateEffectIntensity(Distance);

	if (Intensity <= 0.0f)
	{
		return;
	}

	// Reseta os timers e ativa os efeitos
	ResetPostProcessTimers();
	bPostProcessEffectsActive = true;

	// Chama evento Blueprint
	BP_OnPostProcessApplied(Intensity);

	// Os efeitos serão atualizados no Tick
}

void ALyraExplosionEffect::SpawnExplosionParticles(FVector Location)
{
	// Spawn partículas principais
	if (ExplosionParticles)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ExplosionParticles,
			Location,
			FRotator::ZeroRotator,
			FVector(ParticleScale),
			true,
			true,
			ENCPoolMethod::None,
			true
		);
	}

	// Spawn partículas secundárias com delay
	if (SecondaryParticles && SecondaryParticlesDelay > 0.0f)
	{
		FTimerHandle SecondaryParticlesTimerHandle;
		GetWorldTimerManager().SetTimer(
			SecondaryParticlesTimerHandle,
			[this, Location]()
			{
				if (SecondaryParticles)
				{
					UNiagaraFunctionLibrary::SpawnSystemAtLocation(
						GetWorld(),
						SecondaryParticles,
						Location,
						FRotator::ZeroRotator,
						FVector(ParticleScale),
						true,
						true,
						ENCPoolMethod::None,
						true
					);
				}
			},
			SecondaryParticlesDelay,
			false
		);
	}
	else if (SecondaryParticles)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			SecondaryParticles,
			Location,
			FRotator::ZeroRotator,
			FVector(ParticleScale),
			true,
			true,
			ENCPoolMethod::None,
			true
		);
	}
}

void ALyraExplosionEffect::PlayExplosionSounds(FVector Location)
{
	// Toca som principal da explosão
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			ExplosionSound,
			Location,
			ExplosionSoundVolume
		);
	}

	// Toca som de debris com delay
	if (DebrisSound && DebrisSoundDelay > 0.0f)
	{
		FTimerHandle DebrisSoundTimerHandle;
		GetWorldTimerManager().SetTimer(
			DebrisSoundTimerHandle,
			[this, Location]()
			{
				if (DebrisSound)
				{
					UGameplayStatics::PlaySoundAtLocation(
						GetWorld(),
						DebrisSound,
						Location
					);
				}
			},
			DebrisSoundDelay,
			false
		);
	}
	else if (DebrisSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			DebrisSound,
			Location
		);
	}
}

void ALyraExplosionEffect::ApplyRadialForce(FVector Location)
{
	if (ExplosionForce <= 0.0f || ExplosionRadius <= 0.0f)
	{
		return;
	}

	// Aplica impulso radial a todos os objetos físicos na área
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	
	// Faz overlap sphere para encontrar todos os objetos na área
	GetWorld()->OverlapMultiByChannel(
		Overlaps,
		Location,
		FQuat::Identity,
		ECC_PhysicsBody,
		FCollisionShape::MakeSphere(ExplosionRadius),
		QueryParams
	);

	// Aplica impulso a cada componente primitivo com física
	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (UPrimitiveComponent* PrimComp = Overlap.GetComponent())
		{
			if (PrimComp->IsSimulatingPhysics())
			{
				FVector Direction = PrimComp->GetComponentLocation() - Location;
				float Distance = Direction.Size();
				
				if (Distance > 0.0f)
				{
					Direction.Normalize();
					
					// Calcula força com falloff se habilitado
					float ForceMagnitude = ExplosionForce;
					if (bApplyForceFalloff && Distance > 0.0f)
					{
						float Falloff = 1.0f - FMath::Clamp(Distance / ExplosionRadius, 0.0f, 1.0f);
						ForceMagnitude *= Falloff;
					}
					
					// Aplica o impulso
					PrimComp->AddRadialImpulse(
						Location,
						ExplosionRadius,
						ForceMagnitude,
						bApplyForceFalloff ? ERadialImpulseFalloff::RIF_Linear : ERadialImpulseFalloff::RIF_Constant,
						false // VelChange
					);
				}
			}
		}
	}

	// Debug visual
	if (bShowDebug)
	{
		DrawDebugSphere(
			GetWorld(),
			Location,
			ExplosionRadius,
			32,
			FColor::Orange,
			false,
			2.0f,
			0,
			5.0f
		);
	}
}

float ALyraExplosionEffect::CalculateEffectIntensity(float Distance) const
{
	if (Distance >= MaxEffectDistance)
	{
		return 0.0f;
	}

	float NormalizedDistance = Distance / MaxEffectDistance;
	return FMath::Clamp(1.0f - NormalizedDistance, 0.0f, 1.0f);
}

void ALyraExplosionEffect::StopAllPostProcessEffects()
{
	if (!PostProcessMPC)
	{
		return;
	}

	UMaterialParameterCollectionInstance* MPCInstance = GetWorld()->GetParameterCollectionInstance(PostProcessMPC);
	if (MPCInstance)
	{
		MPCInstance->SetScalarParameterValue(BloomParameterName, 0.0f);
		MPCInstance->SetScalarParameterValue(ChromaticAberrationParameterName, 0.0f);
		MPCInstance->SetScalarParameterValue(VignetteParameterName, 0.0f);
		MPCInstance->SetScalarParameterValue(DesaturationParameterName, 0.0f);
	}

	bPostProcessEffectsActive = false;
	ResetPostProcessTimers();
}

void ALyraExplosionEffect::UpdatePostProcessEffects(float DeltaTime)
{
	if (!PostProcessMPC)
	{
		return;
	}

	UMaterialParameterCollectionInstance* MPCInstance = GetWorld()->GetParameterCollectionInstance(PostProcessMPC);
	if (!MPCInstance)
	{
		return;
	}

	bool bAnyEffectActive = false;

	// Atualiza Bloom
	if (CurrentBloomTime < BloomDuration)
	{
		CurrentBloomTime += DeltaTime;
		float NormalizedTime = FMath::Clamp(CurrentBloomTime / BloomDuration, 0.0f, 1.0f);
		float BloomValue = MaxBloomIntensity;

		if (BloomCurve)
		{
			BloomValue *= BloomCurve->GetFloatValue(NormalizedTime);
		}
		else
		{
			// Fade out padrão
			BloomValue *= (1.0f - NormalizedTime);
		}

		MPCInstance->SetScalarParameterValue(BloomParameterName, BloomValue);
		bAnyEffectActive = true;
	}
	else
	{
		MPCInstance->SetScalarParameterValue(BloomParameterName, 0.0f);
	}

	// Atualiza Chromatic Aberration
	if (CurrentChromaticAberrationTime < ChromaticAberrationDuration)
	{
		CurrentChromaticAberrationTime += DeltaTime;
		float NormalizedTime = FMath::Clamp(CurrentChromaticAberrationTime / ChromaticAberrationDuration, 0.0f, 1.0f);
		float ChromaticValue = MaxChromaticAberration * (1.0f - NormalizedTime);

		MPCInstance->SetScalarParameterValue(ChromaticAberrationParameterName, ChromaticValue);
		bAnyEffectActive = true;
	}
	else
	{
		MPCInstance->SetScalarParameterValue(ChromaticAberrationParameterName, 0.0f);
	}

	// Atualiza Vignette
	if (CurrentVignetteTime < VignetteDuration)
	{
		CurrentVignetteTime += DeltaTime;
		float NormalizedTime = FMath::Clamp(CurrentVignetteTime / VignetteDuration, 0.0f, 1.0f);
		float VignetteValue = MaxVignetteIntensity * (1.0f - NormalizedTime);

		MPCInstance->SetScalarParameterValue(VignetteParameterName, VignetteValue);
		bAnyEffectActive = true;
	}
	else
	{
		MPCInstance->SetScalarParameterValue(VignetteParameterName, 0.0f);
	}

	// Atualiza Desaturation
	if (CurrentDesaturationTime < DesaturationDuration)
	{
		CurrentDesaturationTime += DeltaTime;
		float NormalizedTime = FMath::Clamp(CurrentDesaturationTime / DesaturationDuration, 0.0f, 1.0f);
		float DesaturationValue = MaxDesaturation * (1.0f - NormalizedTime);

		MPCInstance->SetScalarParameterValue(DesaturationParameterName, DesaturationValue);
		bAnyEffectActive = true;
	}
	else
	{
		MPCInstance->SetScalarParameterValue(DesaturationParameterName, 0.0f);
	}

	// Desativa se nenhum efeito está ativo
	if (!bAnyEffectActive)
	{
		bPostProcessEffectsActive = false;
	}
}

void ALyraExplosionEffect::ResetPostProcessTimers()
{
	CurrentBloomTime = 0.0f;
	CurrentChromaticAberrationTime = 0.0f;
	CurrentVignetteTime = 0.0f;
	CurrentDesaturationTime = 0.0f;
}

void ALyraExplosionEffect::DrawDebugVisuals(FVector Location, float Distance, float Intensity)
{
	// Desenha linha do jogador até a explosão
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC && PC->GetPawn())
	{
		FVector PlayerLocation = PC->GetPawn()->GetActorLocation();
		DrawDebugLine(
			GetWorld(),
			PlayerLocation,
			Location,
			FColor::Red,
			false,
			2.0f,
			0,
			3.0f
		);

		// Desenha esfera na explosão
		DrawDebugSphere(
			GetWorld(),
			Location,
			100.0f,
			16,
			FColor::Yellow,
			false,
			2.0f,
			0,
			5.0f
		);

		// Desenha esfera de efeito máximo
		DrawDebugSphere(
			GetWorld(),
			Location,
			MaxEffectDistance,
			32,
			FColor::Green,
			false,
			2.0f,
			0,
			2.0f
		);

		// Mostra texto de debug
		FString DebugText = FString::Printf(TEXT("Distance: %.0f\nIntensity: %.2f"), Distance, Intensity);
		DrawDebugString(
			GetWorld(),
			Location + FVector(0, 0, 200),
			DebugText,
			nullptr,
			FColor::White,
			2.0f,
			true
		);
	}
}

void ALyraExplosionEffect::HandleAutoDestroy()
{
	// Para todos os efeitos antes de destruir
	StopAllPostProcessEffects();
	
	// Destroi o actor
	Destroy();
}
