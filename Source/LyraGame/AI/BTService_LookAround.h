// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_LookAround.generated.h"

/**
 * Serviço que faz o bot olhar em volta enquanto patrulha ou busca pelo jogador
 * Rotaciona a visão do bot em direções aleatórias periodicamente
 */
UCLASS()
class LYRAGAME_API UBTService_LookAround : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_LookAround();

	virtual uint16 GetInstanceMemorySize() const override;

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;

	/** Tempo mínimo entre mudanças de direção do olhar (em segundos) */
	UPROPERTY(EditAnywhere, Category = "Look Around", meta = (ClampMin = "0.5"))
	float MinTimeBetweenLooks;

	/** Tempo máximo entre mudanças de direção do olhar (em segundos) */
	UPROPERTY(EditAnywhere, Category = "Look Around", meta = (ClampMin = "0.5"))
	float MaxTimeBetweenLooks;

	/** Ângulo máximo de rotação horizontal (yaw) em graus */
	UPROPERTY(EditAnywhere, Category = "Look Around", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float MaxYawAngle;

	/** Ângulo máximo de rotação vertical (pitch) em graus */
	UPROPERTY(EditAnywhere, Category = "Look Around", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float MaxPitchAngle;

	/** Velocidade de rotação da visão (graus por segundo) */
	UPROPERTY(EditAnywhere, Category = "Look Around", meta = (ClampMin = "10.0"))
	float RotationSpeed;

	/** Se verdadeiro, usa interpolação constante (mais previsível). Se falso, usa interpolação suave (mais natural) */
	UPROPERTY(EditAnywhere, Category = "Look Around")
	bool bUseConstantRotationSpeed;

	/** Se verdadeiro, apenas rotaciona o controller (visão), se falso rotaciona o corpo inteiro */
	UPROPERTY(EditAnywhere, Category = "Look Around")
	bool bRotateControllerOnly;

	/** Nome da Blackboard Key para armazenar o foco atual (opcional) */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName FocusTargetKey;

private:
	struct FLookAroundMemory
	{
		float TimeUntilNextLook;
		FRotator TargetRotation;
		FRotator StartRotation;
		float LookProgress;
		bool bHasTarget;

		FLookAroundMemory()
			: TimeUntilNextLook(0.f)
			, TargetRotation(FRotator::ZeroRotator)
			, StartRotation(FRotator::ZeroRotator)
			, LookProgress(1.f)
			, bHasTarget(false)
		{
		}
	};

	void GenerateNewLookTarget(FLookAroundMemory* Memory, AAIController* AIController);
};

