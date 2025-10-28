// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SetFocusToRandomDirection.generated.h"

/**
 * Task que define uma direção aleatória para o bot focar temporariamente
 * Útil para fazer o bot olhar em volta durante patrulhas
 */
UCLASS()
class LYRAGAME_API UBTTask_SetFocusToRandomDirection : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SetFocusToRandomDirection();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	/** Distância do ponto focal à frente do bot */
	UPROPERTY(EditAnywhere, Category = "Focus", meta = (ClampMin = "100.0"))
	float FocusDistance;

	/** Ângulo máximo de desvio horizontal (yaw) em graus */
	UPROPERTY(EditAnywhere, Category = "Focus", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float MaxYawDeviation;

	/** Ângulo máximo de desvio vertical (pitch) em graus */
	UPROPERTY(EditAnywhere, Category = "Focus", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float MaxPitchDeviation;

	/** Se verdadeiro, limpa o foco anterior antes de definir o novo */
	UPROPERTY(EditAnywhere, Category = "Focus")
	bool bClearPreviousFocus;
};

