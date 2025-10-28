// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/BTService_LookAround.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UBTService_LookAround::UBTService_LookAround()
{
	NodeName = "Look Around While Patrolling";
	bNotifyTick = true;
	
	MinTimeBetweenLooks = 2.0f;
	MaxTimeBetweenLooks = 5.0f;
	MaxYawAngle = 90.0f;
	MaxPitchAngle = 30.0f;
	RotationSpeed = 45.0f;  // Reduzido de 90 para 45 graus/segundo
	bUseConstantRotationSpeed = true;  // Velocidade constante por padrão (mais previsível)
	bRotateControllerOnly = false;
	FocusTargetKey = NAME_None;
}

uint16 UBTService_LookAround::GetInstanceMemorySize() const
{
	return sizeof(FLookAroundMemory);
}

void UBTService_LookAround::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	FLookAroundMemory* Memory = reinterpret_cast<FLookAroundMemory*>(NodeMemory);
	if (Memory)
	{
		new (Memory) FLookAroundMemory();
	}
}

void UBTService_LookAround::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return;
	}

	// Verifica se há um alvo de foco na blackboard (prioridade sobre o look around)
	if (!FocusTargetKey.IsNone())
	{
		UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
		if (BlackboardComp)
		{
			UObject* FocusTarget = BlackboardComp->GetValueAsObject(FocusTargetKey);
			if (FocusTarget)
			{
				// Se há um alvo de foco, não faz look around
				return;
			}
		}
	}

	APawn* ControlledPawn = AIController->GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	FLookAroundMemory* Memory = reinterpret_cast<FLookAroundMemory*>(NodeMemory);
	if (!Memory)
	{
		return;
	}

	// Atualiza o temporizador
	Memory->TimeUntilNextLook -= DeltaSeconds;

	// Se é hora de gerar um novo alvo de olhar
	if (Memory->TimeUntilNextLook <= 0.0f)
	{
		GenerateNewLookTarget(Memory, AIController);
	}

	// Interpola suavemente para a rotação alvo usando velocidade angular
	if (Memory->bHasTarget && Memory->LookProgress < 1.0f)
	{
		FRotator CurrentRotation;
		
		if (bUseConstantRotationSpeed)
		{
			// Interpolação constante - rotação a velocidade fixa (mais previsível)
			CurrentRotation = FMath::RInterpConstantTo(
				Memory->StartRotation,
				Memory->TargetRotation,
				DeltaSeconds,
				RotationSpeed  // Graus por segundo
			);
		}
		else
		{
			// Interpolação suave - acelera e desacelera (mais natural)
			float InterpSpeed = RotationSpeed / 45.0f;  // Normaliza para taxa de interpolação
			CurrentRotation = FMath::RInterpTo(
				Memory->StartRotation,
				Memory->TargetRotation,
				DeltaSeconds,
				InterpSpeed
			);
		}
		
		// Atualiza a rotação inicial para a próxima iteração
		Memory->StartRotation = CurrentRotation;
		
		// Verifica se chegou próximo o suficiente do alvo
		float AngleDifference = FMath::Abs(FMath::FindDeltaAngleDegrees(
			CurrentRotation.Yaw, 
			Memory->TargetRotation.Yaw
		));
		
		if (AngleDifference < 1.0f)
		{
			Memory->LookProgress = 1.0f;
			Memory->StartRotation = Memory->TargetRotation;
		}
		else
		{
			Memory->LookProgress += DeltaSeconds * 0.1f; // Progresso gradual
		}

		if (bRotateControllerOnly)
		{
			// Apenas rotaciona o controller (a visão)
			AIController->SetControlRotation(CurrentRotation);
		}
		else
		{
			// Rotaciona o corpo do personagem
			ACharacter* Character = Cast<ACharacter>(ControlledPawn);
			if (Character)
			{
				// Usa o movimento do personagem para rotacionar suavemente
				UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
				if (MovementComp && !MovementComp->Velocity.IsNearlyZero())
				{
					// Se está se movendo, apenas rotaciona o controller
					AIController->SetControlRotation(CurrentRotation);
				}
				else
				{
					// Se está parado, rotaciona o corpo inteiro
					Character->SetActorRotation(FRotator(0.0f, CurrentRotation.Yaw, 0.0f));
					AIController->SetControlRotation(CurrentRotation);
				}
			}
			else
			{
				ControlledPawn->SetActorRotation(FRotator(0.0f, CurrentRotation.Yaw, 0.0f));
				AIController->SetControlRotation(CurrentRotation);
			}
		}
	}
}

void UBTService_LookAround::GenerateNewLookTarget(FLookAroundMemory* Memory, AAIController* AIController)
{
	if (!AIController || !AIController->GetPawn())
	{
		return;
	}

	// Salva a rotação atual como ponto de partida
	Memory->StartRotation = AIController->GetControlRotation();
	Memory->LookProgress = 0.0f;
	Memory->bHasTarget = true;

	// Gera um tempo aleatório até o próximo look
	Memory->TimeUntilNextLook = FMath::RandRange(MinTimeBetweenLooks, MaxTimeBetweenLooks);

	// Gera uma rotação aleatória baseada na rotação atual do pawn
	FRotator BaseRotation = AIController->GetPawn()->GetActorRotation();
	
	// Ângulo horizontal aleatório (yaw)
	float RandomYaw = FMath::RandRange(-MaxYawAngle, MaxYawAngle);
	
	// Ângulo vertical aleatório (pitch)
	float RandomPitch = FMath::RandRange(-MaxPitchAngle, MaxPitchAngle);

	// Define a rotação alvo
	Memory->TargetRotation = FRotator(
		FMath::ClampAngle(BaseRotation.Pitch + RandomPitch, -MaxPitchAngle, MaxPitchAngle),
		FMath::ClampAngle(BaseRotation.Yaw + RandomYaw, -180.0f, 180.0f),
		0.0f
	);
}

