

#include "LyraGameplayAbility_AutoCover.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "Character/LyraCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameplayTagContainer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraGameplayAbility_AutoCover)

ULyraGameplayAbility_AutoCover::ULyraGameplayAbility_AutoCover(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// Auto-activation settings
	bAutoActivate = true;
	AutoActivationCheckRate = 0.1f; // Check 10 times per second

	// Default cover settings
	CoverDetectionDistance = 150.0f;
	CoverDetectionRadius = 30.0f;
	DistanceFromCover = 50.0f;
	CoverMovementSpeed = 300.0f;
	EdgeDetectionDistance = 100.0f;
	MinimumForwardInput = 0.5f;
	ExitCoverBackwardThreshold = -0.3f; // Exit when moving backwards
	bOnlyAimAtEdges = true;
	CoverAttachSpeed = 10.0f;
	CoverTag = FName("cover"); // Default component tag name

	// Initialize state
	CurrentCoverActor = nullptr;
	CurrentCoverComponent = nullptr;
	CoverNormal = FVector::ZeroVector;
	CoverAttachPoint = FVector::ZeroVector;
	bIsAtEdge = false;
	bIsRightEdge = false;
	bHasCachedCoverHit = false;
}

void ULyraGameplayAbility_AutoCover::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	// Start auto-activation checks if enabled
	if (bAutoActivate && ActorInfo && ActorInfo->IsLocallyControlled())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				AutoActivationCheckTimerHandle,
				this,
				&ULyraGameplayAbility_AutoCover::CheckAutoActivation,
				AutoActivationCheckRate,
				true // Loop
			);
		}
	}
}

bool ULyraGameplayAbility_AutoCover::TryActivateCoverAbility()
{
	// Try to activate the ability
	if (CanActivateAbility(CurrentSpecHandle, CurrentActorInfo, nullptr, nullptr, nullptr))
	{
		CallActivateAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, nullptr, nullptr);
		return true;
	}
	return false;
}

void ULyraGameplayAbility_AutoCover::CheckAutoActivation()
{
	// Don't try to activate if already active
	if (IsActive())
	{
		return;
	}

	// Check if conditions are met and try to activate
	if (CanActivateAbility(CurrentSpecHandle, CurrentActorInfo, nullptr, nullptr, nullptr))
	{
		TryActivateCoverAbility();
	}
}

bool ULyraGameplayAbility_AutoCover::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		bHasCachedCoverHit = false;
		return false;
	}

	const ALyraCharacter* LyraCharacter = GetLyraCharacterFromActorInfo();
	if (!LyraCharacter)
	{
		bHasCachedCoverHit = false;
		return false;
	}

	// Check if there's a valid cover in front first (faster check)
	FHitResult HitResult;
	if (!CheckForCoverInFront(HitResult))
	{
		bHasCachedCoverHit = false;
		return false;
	}

	// Cache the hit result for use in ActivateAbility
	CachedCoverHit = HitResult;
	bHasCachedCoverHit = true;

	// Check if moving towards cover (must be moving forward)
	if (!IsMovingTowardsCover())
	{
		bHasCachedCoverHit = false;
		return false;
	}

	return true;
}

void ULyraGameplayAbility_AutoCover::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Use cached cover hit from CanActivateAbility if available
	// This prevents the issue where re-checking fails because conditions changed
	if (bHasCachedCoverHit)
	{
		AttachToCover(CachedCoverHit);
		K2_OnEnterCover(CachedCoverHit);
		
		// Clear cache after use
		bHasCachedCoverHit = false;

		// Start update timer
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				CoverUpdateTimerHandle,
				[this]()
				{
					if (IsActive())
					{
						float DeltaTime = GetWorld()->GetDeltaSeconds();
						UpdateCoverMovement(DeltaTime);
						K2_OnCoverTick(DeltaTime);
					}
				},
				0.016f, // ~60 FPS
				true
			);
		}
	}
	else
	{
		// Fallback: try to find cover (shouldn't normally happen)
		FHitResult CoverHit;
		if (CheckForCoverInFront(CoverHit))
		{
			AttachToCover(CoverHit);
			K2_OnEnterCover(CoverHit);

			// Start update timer
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(
					CoverUpdateTimerHandle,
					[this]()
					{
						if (IsActive())
						{
							float DeltaTime = GetWorld()->GetDeltaSeconds();
							UpdateCoverMovement(DeltaTime);
							K2_OnCoverTick(DeltaTime);
						}
					},
					0.016f, // ~60 FPS
					true
				);
			}
		}
		else
		{
			// No valid cover found, end ability
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		}
	}
}

void ULyraGameplayAbility_AutoCover::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Clear update timer
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CoverUpdateTimerHandle);
	}

	DetachFromCover();
	K2_OnExitCover();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	
	// Note: We do NOT clear AutoActivationCheckTimerHandle here
	// It should keep running to allow re-entering cover
}

bool ULyraGameplayAbility_AutoCover::CheckForCoverInFront(FHitResult& OutHitResult) const
{
	const ALyraCharacter* LyraCharacter = GetLyraCharacterFromActorInfo();
	if (!LyraCharacter)
	{
		return false;
	}

	// Get character forward vector and location
	const FVector CharacterLocation = LyraCharacter->GetActorLocation();
	const FVector ForwardVector = LyraCharacter->GetActorForwardVector();
	const FVector StartLocation = CharacterLocation;
	const FVector EndLocation = StartLocation + (ForwardVector * CoverDetectionDistance);

	// Perform sphere trace
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(const_cast<ALyraCharacter*>(LyraCharacter));

	bool bHit = UKismetSystemLibrary::SphereTraceSingle(
		GetWorld(),
		StartLocation,
		EndLocation,
		CoverDetectionRadius,
		UEngineTypes::ConvertToTraceType(ECC_Visibility),
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		OutHitResult,
		true
	);

	if (bHit && OutHitResult.GetComponent())
	{
		// Check if hit component has cover tag
		if (!CoverTag.IsNone() && OutHitResult.GetComponent()->ComponentHasTag(CoverTag))
		{
			return true;
		}
		
		// Also check actor tags as fallback
		if (OutHitResult.GetActor() && OutHitResult.GetActor()->ActorHasTag(CoverTag))
		{
			return true;
		}
	}

	return false;
}

bool ULyraGameplayAbility_AutoCover::IsMovingTowardsCover() const
{
	const ALyraCharacter* LyraCharacter = GetLyraCharacterFromActorInfo();
	if (!LyraCharacter)
	{
		return false;
	}

	// Get movement input
	const UCharacterMovementComponent* MovementComponent = LyraCharacter->GetCharacterMovement();
	if (!MovementComponent)
	{
		return false;
	}

	// Get input vector in character's local space
	const FVector InputVector = MovementComponent->GetLastInputVector();
	if (InputVector.IsNearlyZero())
	{
		return false;
	}

	// Check if moving forward (dot product with forward vector)
	const FVector ForwardVector = LyraCharacter->GetActorForwardVector();
	const float ForwardDot = FVector::DotProduct(InputVector.GetSafeNormal(), ForwardVector);

	return ForwardDot >= MinimumForwardInput;
}

void ULyraGameplayAbility_AutoCover::AttachToCover(const FHitResult& CoverHit)
{
	ALyraCharacter* LyraCharacter = GetLyraCharacterFromActorInfo();
	if (!LyraCharacter)
	{
		return;
	}

	// Store cover information
	CurrentCoverActor = CoverHit.GetActor();
	CurrentCoverComponent = CoverHit.GetComponent();
	CoverNormal = CoverHit.Normal;
	
	// Calculate attach point (offset from wall)
	CoverAttachPoint = CoverHit.ImpactPoint + (CoverNormal * DistanceFromCover);

	// Rotate character to face away from cover
	const FRotator CoverRotation = UKismetMathLibrary::MakeRotFromX(-CoverNormal);
	LyraCharacter->SetActorRotation(CoverRotation);
}

void ULyraGameplayAbility_AutoCover::DetachFromCover()
{
	ALyraCharacter* LyraCharacter = GetLyraCharacterFromActorInfo();
	if (!LyraCharacter)
	{
		return;
	}

	// Clear cover information
	CurrentCoverActor = nullptr;
	CurrentCoverComponent = nullptr;
	CoverNormal = FVector::ZeroVector;
	CoverAttachPoint = FVector::ZeroVector;
	bIsAtEdge = false;
	bIsRightEdge = false;
}

void ULyraGameplayAbility_AutoCover::UpdateCoverMovement(float DeltaTime)
{
	ALyraCharacter* LyraCharacter = GetLyraCharacterFromActorInfo();
	if (!LyraCharacter || !CurrentCoverActor)
	{
		return;
	}

	// Check if player is moving backwards (away from cover)
	const UCharacterMovementComponent* MovementComponent = LyraCharacter->GetCharacterMovement();
	if (MovementComponent)
	{
		const FVector InputVector = MovementComponent->GetLastInputVector();
		if (!InputVector.IsNearlyZero())
		{
			// Check if input is backwards (negative dot product with character forward)
			const FVector ForwardVector = LyraCharacter->GetActorForwardVector();
			const float ForwardDot = FVector::DotProduct(InputVector.GetSafeNormal(), ForwardVector);
			
			// If moving backwards, exit cover
			if (ForwardDot < ExitCoverBackwardThreshold)
			{
				EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
				return;
			}
		}
	}

	// Get lateral input
	const float LateralInput = GetLateralMovementInput();

	if (!FMath::IsNearlyZero(LateralInput))
	{
		// Calculate right vector along the cover
		const FVector RightVector = FVector::CrossProduct(CoverNormal, FVector::UpVector).GetSafeNormal();
		const FVector MovementDirection = RightVector * LateralInput;
		
		// Calculate new position
		const FVector CurrentLocation = LyraCharacter->GetActorLocation();
		const FVector DesiredMovement = MovementDirection * CoverMovementSpeed * DeltaTime;
		FVector NewLocation = CurrentLocation + DesiredMovement;

		// Trace along the cover to maintain attachment
		FHitResult TraceHit;
		const FVector TraceStart = NewLocation + (CoverNormal * 100.0f);
		const FVector TraceEnd = NewLocation - (CoverNormal * 100.0f);

		TArray<AActor*> ActorsToIgnore;
		ActorsToIgnore.Add(LyraCharacter);

		bool bHit = UKismetSystemLibrary::LineTraceSingle(
			GetWorld(),
			TraceStart,
			TraceEnd,
			UEngineTypes::ConvertToTraceType(ECC_Visibility),
			false,
			ActorsToIgnore,
			EDrawDebugTrace::None,
			TraceHit,
			true
		);

		if (bHit && TraceHit.GetActor() == CurrentCoverActor)
		{
			// Update position along cover
			NewLocation = TraceHit.ImpactPoint + (TraceHit.Normal * DistanceFromCover);
			LyraCharacter->SetActorLocation(NewLocation);
			
			// Update cover normal
			CoverNormal = TraceHit.Normal;
			const FRotator NewRotation = UKismetMathLibrary::MakeRotFromX(-CoverNormal);
			LyraCharacter->SetActorRotation(NewRotation);
			
			// IMPORTANT: Update attach point to new position so character doesn't get pulled back
			CoverAttachPoint = NewLocation;
		}
		else
		{
			// Lost contact with cover, end ability
			K2_CommitExecute();
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
			return;
		}
	}

	// Check edge status
	const bool bWasAtEdge = bIsAtEdge;
	const bool bWasRightEdge = bIsRightEdge;

	const bool bAtRightEdge = IsAtCoverEdge(true);
	const bool bAtLeftEdge = IsAtCoverEdge(false);

	if (bAtRightEdge)
	{
		bIsAtEdge = true;
		bIsRightEdge = true;
	}
	else if (bAtLeftEdge)
	{
		bIsAtEdge = true;
		bIsRightEdge = false;
	}
	else
	{
		bIsAtEdge = false;
	}

	// Notify if edge status changed
	if (bWasAtEdge != bIsAtEdge || bWasRightEdge != bIsRightEdge)
	{
		K2_OnCoverEdgeStatusChanged(bIsAtEdge, bIsRightEdge);
	}
}

bool ULyraGameplayAbility_AutoCover::IsAtCoverEdge(bool bCheckRightEdge) const
{
	const ALyraCharacter* LyraCharacter = GetLyraCharacterFromActorInfo();
	if (!LyraCharacter || !CurrentCoverActor)
	{
		return false;
	}

	// Calculate direction to check
	const FVector RightVector = FVector::CrossProduct(CoverNormal, FVector::UpVector).GetSafeNormal();
	const FVector CheckDirection = bCheckRightEdge ? RightVector : -RightVector;

	// Trace to check for cover continuation
	const FVector StartLocation = LyraCharacter->GetActorLocation() + (CheckDirection * EdgeDetectionDistance);
	const FVector TraceStart = StartLocation + (CoverNormal * 100.0f);
	const FVector TraceEnd = StartLocation - (CoverNormal * 100.0f);

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(const_cast<ALyraCharacter*>(LyraCharacter));

	FHitResult HitResult;
	bool bHit = UKismetSystemLibrary::LineTraceSingle(
		GetWorld(),
		TraceStart,
		TraceEnd,
		UEngineTypes::ConvertToTraceType(ECC_Visibility),
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		HitResult,
		true
	);

	// If we don't hit the same cover actor, we're at an edge
	return !bHit || HitResult.GetActor() != CurrentCoverActor;
}

float ULyraGameplayAbility_AutoCover::GetLateralMovementInput() const
{
	const ALyraCharacter* LyraCharacter = GetLyraCharacterFromActorInfo();
	if (!LyraCharacter)
	{
		return 0.0f;
	}

	const UCharacterMovementComponent* MovementComponent = LyraCharacter->GetCharacterMovement();
	if (!MovementComponent)
	{
		return 0.0f;
	}

	// Get input vector - this is the raw input from player
	FVector InputVector = MovementComponent->GetLastInputVector();
	if (InputVector.IsNearlyZero())
	{
		return 0.0f;
	}

	// Normalize input
	InputVector.Normalize();

	// Calculate right vector along the cover (perpendicular to cover normal)
	const FVector CoverRightVector = FVector::CrossProduct(FVector::UpVector, CoverNormal).GetSafeNormal();
	
	// Project input onto the cover's right vector (lateral movement)
	const float LateralInput = FVector::DotProduct(InputVector, CoverRightVector);

	return LateralInput;
}

bool ULyraGameplayAbility_AutoCover::CanAimFromCover() const
{
	if (bOnlyAimAtEdges)
	{
		return bIsAtEdge;
	}

	return true;
}
