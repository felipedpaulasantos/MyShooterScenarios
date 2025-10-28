// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/BTTask_SetFocusToRandomDirection.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_SetFocusToRandomDirection::UBTTask_SetFocusToRandomDirection()
{
	NodeName = "Set Focus to Random Direction";
	
	FocusDistance = 500.0f;
	MaxYawDeviation = 90.0f;
	MaxPitchDeviation = 30.0f;
	bClearPreviousFocus = true;

	// Esta task executa instantaneamente
	bNotifyTick = false;
}

EBTNodeResult::Type UBTTask_SetFocusToRandomDirection::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	APawn* ControlledPawn = AIController->GetPawn();
	if (!ControlledPawn)
	{
		return EBTNodeResult::Failed;
	}

	// Limpa o foco anterior se solicitado
	if (bClearPreviousFocus)
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	// Obtém a rotação atual do pawn
	FRotator CurrentRotation = ControlledPawn->GetActorRotation();
	
	// Gera desvios aleatórios
	float RandomYaw = FMath::RandRange(-MaxYawDeviation, MaxYawDeviation);
	float RandomPitch = FMath::RandRange(-MaxPitchDeviation, MaxPitchDeviation);
	
	// Cria a rotação alvo
	FRotator TargetRotation = FRotator(
		FMath::ClampAngle(CurrentRotation.Pitch + RandomPitch, -MaxPitchDeviation, MaxPitchDeviation),
		FMath::ClampAngle(CurrentRotation.Yaw + RandomYaw, -180.0f, 180.0f),
		0.0f
	);

	// Calcula a posição do ponto focal baseado na direção alvo
	FVector StartLocation = ControlledPawn->GetActorLocation();
	FVector FocusLocation = StartLocation + (TargetRotation.Vector() * FocusDistance);

	// Define o ponto focal
	AIController->SetFocalPoint(FocusLocation, EAIFocusPriority::Gameplay);

	return EBTNodeResult::Succeeded;
}

FString UBTTask_SetFocusToRandomDirection::GetStaticDescription() const
{
	return FString::Printf(TEXT("Set focus to random direction\nDistance: %.0f\nYaw: ±%.0f°\nPitch: ±%.0f°"), 
		FocusDistance, MaxYawDeviation, MaxPitchDeviation);
}

