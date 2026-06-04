// Copyright MyShooterScenarios. All Rights Reserved.

#include "Pause/MYSTPauseSubsystem.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MYSTPauseSubsystem)

// ---------------------------------------------------------------------------
// USubsystem
// ---------------------------------------------------------------------------

bool UMYSTPauseSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Always create so that Blueprint can always GetSubsystem() safely.
	return true;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void UMYSTPauseSubsystem::RequestPause()
{
	if (!bCurrentlyPaused)
	{
		ApplyPause(true);
	}
}

void UMYSTPauseSubsystem::RequestUnpause()
{
	if (bCurrentlyPaused)
	{
		ApplyPause(false);
	}
}

void UMYSTPauseSubsystem::TogglePause()
{
	ApplyPause(!bCurrentlyPaused);
}

// ---------------------------------------------------------------------------
// Internal
// ---------------------------------------------------------------------------

void UMYSTPauseSubsystem::ApplyPause(bool bPause)
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("MYSTPauseSubsystem: ApplyPause called with no valid World."));
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("MYSTPauseSubsystem: ApplyPause — no PlayerController found."));
		return;
	}

	// ------------------------------------------------------------------
	// 1. Engine world-pause.
	//    APlayerController::SetPause is the correct call — it:
	//      - Respects any CanUnpause delegates registered by other systems.
	//      - Sets UWorld::bIsPaused which stops actor Tick, physics,
	//        timelines, animations, AI BT tasks, and Niagara simulation.
	// ------------------------------------------------------------------
	const bool bSuccess = PC->SetPause(bPause);
	if (!bSuccess)
	{
		// Another system registered a CanUnpause delegate that blocked us.
		UE_LOG(LogTemp, Warning,
			TEXT("MYSTPauseSubsystem: SetPause(%s) was blocked (CanUnpause delegate returned false)."),
			bPause ? TEXT("true") : TEXT("false"));
		return;
	}

	// ------------------------------------------------------------------
	// 2. GAS ability-task suspension.
	//    UWorld::bIsPaused stops actor ticks but the ASC component tick is
	//    marked bTickEvenWhenPaused in some GAS configurations, which lets
	//    ability tasks (e.g. WaitDelay, WaitGameplayEvent) keep running.
	//    Explicitly disabling / re-enabling the ASC's component tick on
	//    every pawn ensures ability logic is fully frozen.
	// ------------------------------------------------------------------
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Pawn = *It;
		if (!IsValid(Pawn))
		{
			continue;
		}

		if (UAbilitySystemComponent* ASC = Pawn->FindComponentByClass<UAbilitySystemComponent>())
		{
			ASC->SetComponentTickEnabled(!bPause);
		}
	}

	bCurrentlyPaused = bPause;
	OnPauseStateChanged.Broadcast(bCurrentlyPaused);

	UE_LOG(LogTemp, Log, TEXT("MYSTPauseSubsystem: Game %s."),
		bCurrentlyPaused ? TEXT("PAUSED") : TEXT("UNPAUSED"));
}



