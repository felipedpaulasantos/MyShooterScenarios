// Copyright MyShooterScenarios. All Rights Reserved.

#include "AI/BTService_PlayerPerception.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Damage.h"

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

UBTService_PlayerPerception::UBTService_PlayerPerception(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = "Player Perception (MYST)";
	bNotifyTick = true;

	// Each AI controller gets its own UObject instance so delegate bindings and
	// cooldown timestamps are stored safely as plain member variables — identical
	// pattern to BTService_AIStateObserver.
	bCreateNodeInstance = true;

	Interval        = 0.1f;
	RandomDeviation = 0.05f;

	HasSeenPlayerKey.AddBoolFilter(
		this, GET_MEMBER_NAME_CHECKED(UBTService_PlayerPerception, HasSeenPlayerKey));
	TargetEnemyKey.AddObjectFilter(
		this, GET_MEMBER_NAME_CHECKED(UBTService_PlayerPerception, TargetEnemyKey), AActor::StaticClass());
	LastKnownLocationKey.AddVectorFilter(
		this, GET_MEMBER_NAME_CHECKED(UBTService_PlayerPerception, LastKnownLocationKey));
	HasHeardPlayerKey.AddBoolFilter(
		this, GET_MEMBER_NAME_CHECKED(UBTService_PlayerPerception, HasHeardPlayerKey));
}

// ─────────────────────────────────────────────────────────────────────────────
// Asset initialisation
// ─────────────────────────────────────────────────────────────────────────────

void UBTService_PlayerPerception::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		HasSeenPlayerKey.ResolveSelectedKey(*BBAsset);
		TargetEnemyKey.ResolveSelectedKey(*BBAsset);
		LastKnownLocationKey.ResolveSelectedKey(*BBAsset);
		HasHeardPlayerKey.ResolveSelectedKey(*BBAsset);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Become / Cease Relevant
// ─────────────────────────────────────────────────────────────────────────────

void UBTService_PlayerPerception::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	// Reset all per-activation state so a previous activation cycle doesn't
	// leave stale timestamps.
	LastDetectionTime     = -1.f;
	LastHearTime          = -1.f;
	bCachedHasSeenPlayer  = false;
	bCachedHasHeardPlayer = false;
	bLocationIsSet        = false;
	TrackedTargetPawn     = nullptr;
	OwnerBTComp           = &OwnerComp;

	// Snap keys to clean state so no stale values linger from a previous cycle.
	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		if (HasSeenPlayerKey.IsSet())   { BB->SetValueAsBool(HasSeenPlayerKey.SelectedKeyName, false); }
		if (HasHeardPlayerKey.IsSet())  { BB->SetValueAsBool(HasHeardPlayerKey.SelectedKeyName, false); }
		if (TargetEnemyKey.IsSet())     { BB->ClearValue(TargetEnemyKey.SelectedKeyName); }
	}

	// Eager bind — may fail if the controller isn't fully initialised yet;
	// TickNode retries lazily on every tick until it succeeds.
	// Sync immediately after a fresh bind so actors already in sight are picked
	// up even though OnTargetPerceptionUpdated won't fire retroactively.
	if (AAIController* AIC = OwnerComp.GetAIOwner())
	{
		if (TryBindPerceptionDelegates(AIC))
		{
			SyncCurrentPerceptionState(OwnerComp);
		}
	}
}

void UBTService_PlayerPerception::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UnbindPerceptionDelegates();
	OwnerBTComp = nullptr;

	Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}

// ─────────────────────────────────────────────────────────────────────────────
// BeginDestroy — last-resort delegate cleanup
// ─────────────────────────────────────────────────────────────────────────────

void UBTService_PlayerPerception::BeginDestroy()
{
	// OnCeaseRelevant is the normal cleanup path.  This covers the edge case
	// where GC collects the node instance before the BT shuts down cleanly.
	// Without this, UAIPerceptionComponent::OnTargetPerceptionUpdated retains
	// a dangling binding and will crash the next time it fires.
	UnbindPerceptionDelegates();

	Super::BeginDestroy();
}

// ─────────────────────────────────────────────────────────────────────────────
// Perception delegate callback — single entry-point for all senses
// ─────────────────────────────────────────────────────────────────────────────

void UBTService_PlayerPerception::HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// Only react to fresh, successfully-sensed stimuli. The perception system
	// also fires this with WasSuccessfullySensed()==false when a stimulus ages
	// out — we deliberately ignore those because our own cooldown timers in
	// TickNode drive the decay, giving us independent control over durations.
	if (!Actor || !Stimulus.WasSuccessfullySensed())
	{
		return;
	}

	// Bail out during world teardown.  The perception component fires stimuli
	// during EndPlay processing (e.g. sight/damage events on actor destroy).
	// OwnerBTComp may still be non-null at that point, so this is an explicit
	// guard rather than relying on the weak-pointer check below.
	{
		UWorld* World = GetWorld();
		if (!World || World->bIsTearingDown) { return; }
	}

	// Optional extra filter: skip non-player pawns for sight/hearing.
	// For damage we apply the filter inside OnDamageStimulus so we can react
	// even when the attacker pawn reference is null (e.g. a remote projectile).
	APawn* DetectedPawn = Cast<APawn>(Actor);

	UBehaviorTreeComponent* BTComp = OwnerBTComp.Get();
	if (!BTComp) { return; }

	UBlackboardComponent* BB = BTComp->GetBlackboardComponent();
	if (!BB) { return; }

	const FAISenseID SightID   = UAISense::GetSenseID<UAISense_Sight>();
	const FAISenseID HearingID = UAISense::GetSenseID<UAISense_Hearing>();
	const FAISenseID DamageID  = UAISense::GetSenseID<UAISense_Damage>();

	if (Stimulus.Type == SightID)
	{
		if (bOnlyDetectPlayers && (!DetectedPawn || !DetectedPawn->IsPlayerControlled())) { return; }
		OnSightStimulus(DetectedPawn, Stimulus.StimulusLocation, BB);
	}
	else if (Stimulus.Type == HearingID)
	{
		if (bOnlyDetectPlayers && (!DetectedPawn || !DetectedPawn->IsPlayerControlled())) { return; }
		OnHearingStimulus(DetectedPawn, Stimulus.StimulusLocation, Stimulus.Strength, BB);
	}
	else if (Stimulus.Type == DamageID)
	{
		OnDamageStimulus(DetectedPawn, Stimulus.StimulusLocation, BB);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Per-sense helpers
// ─────────────────────────────────────────────────────────────────────────────

void UBTService_PlayerPerception::OnSightStimulus(APawn* SeenPawn, const FVector& Location, UBlackboardComponent* BB)
{
	// Refresh the master detection timestamp on every sight confirmation so the
	// cooldown always measures from the last actual sighting.
	if (UWorld* World = GetWorld())
	{
		LastDetectionTime = World->GetTimeSeconds();
	}

	// HasSeenPlayer → true (instant, write-only-on-change).
	const bool bPreviouslySeen = bCachedHasSeenPlayer;
	SetBBBool(BB, HasSeenPlayerKey, true, bCachedHasSeenPlayer);

	// TargetEnemy — always refresh to the current seen pawn.
	if (TargetEnemyKey.IsSet())
	{
		BB->SetValueAsObject(TargetEnemyKey.SelectedKeyName, SeenPawn);
		TrackedTargetPawn = SeenPawn;
	}

	// LastKnownLocation — always refresh while actively seeing the target.
	// UAIPerceptionComponent fires this every perception tick while actor is in
	// sight, so this stays current without any additional polling.
	if (LastKnownLocationKey.IsSet())
	{
		// Use the actor's current world location for the highest precision; the
		// StimulusLocation in FAIStimulus is the location at the moment of detection.
		BB->SetValueAsVector(LastKnownLocationKey.SelectedKeyName,
			SeenPawn ? SeenPawn->GetActorLocation() : Location);
		bLocationIsSet = true;
	}

	// Fire the Blueprint hook only on the false → true edge.
	if (!bPreviouslySeen)
	{
		OnPlayerFirstSpotted(SeenPawn);
	}
}

void UBTService_PlayerPerception::OnHearingStimulus(APawn* NoiseMaker, const FVector& Location, float Loudness, UBlackboardComponent* BB)
{
	if (Loudness < HearingLoudnessThreshold)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		LastHearTime = World->GetTimeSeconds();
		// Hearing refreshes the master detection timer so the location stays
		// valid as long as the AI has recently heard (or seen) the player.
		LastDetectionTime = FMath::Max(LastDetectionTime, LastHearTime);
	}

	// Update last known location to the heard position.
	if (LastKnownLocationKey.IsSet())
	{
		BB->SetValueAsVector(LastKnownLocationKey.SelectedKeyName, Location);
		bLocationIsSet = true;
	}

	// If we have a valid noise-making pawn, update TargetEnemy as well so
	// hearing alone can populate the target reference (same expiry as location).
	if (NoiseMaker && TargetEnemyKey.IsSet() && !TrackedTargetPawn.IsValid())
	{
		BB->SetValueAsObject(TargetEnemyKey.SelectedKeyName, NoiseMaker);
		TrackedTargetPawn = NoiseMaker;
	}

	// Arm HasHeardPlayer (write-only-on-change).
	SetBBBool(BB, HasHeardPlayerKey, true, bCachedHasHeardPlayer);

	OnNoiseHeard(NoiseMaker, Location, Loudness);
}

// ─────────────────────────────────────────────────────────────────────────────

void UBTService_PlayerPerception::OnDamageStimulus(APawn* AttackerPawn, const FVector& InstigatorLocation, UBlackboardComponent* BB)
{
	// Apply player-only filter: skip if bOnlyDamageByPlayers and the instigator
	// is not a player-controlled pawn.  We check here rather than in the dispatch
	// so that environmental damage (null pawn) can still be optionally allowed.
	if (bOnlyDamageByPlayers && (!AttackerPawn || !AttackerPawn->IsPlayerControlled()))
	{
		return;
	}

	// Refresh the master detection timer so location retention runs from the
	// latest damage event, not just the last sight/hearing event.
	if (UWorld* World = GetWorld())
	{
		LastDetectionTime = FMath::Max(LastDetectionTime, World->GetTimeSeconds());
	}

	// Set LastKnownLocation to the instigator's recorded position so the BT
	// can navigate / investigate the direction from which the damage came.
	if (LastKnownLocationKey.IsSet())
	{
		BB->SetValueAsVector(LastKnownLocationKey.SelectedKeyName, InstigatorLocation);
		bLocationIsSet = true;
	}

	// Populate TargetEnemy with the attacker pawn when available and when we
	// don't already have a higher-confidence sight target.
	if (AttackerPawn && TargetEnemyKey.IsSet() && !TrackedTargetPawn.IsValid())
	{
		BB->SetValueAsObject(TargetEnemyKey.SelectedKeyName, AttackerPawn);
		TrackedTargetPawn = AttackerPawn;
	}


	OnDamageTaken(AttackerPawn, InstigatorLocation);
}

// ─────────────────────────────────────────────────────────────────────────────
// Tick — cooldown expiry only; detection is fully delegate-driven
// ─────────────────────────────────────────────────────────────────────────────

void UBTService_PlayerPerception::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	// ── Lazy bind ─────────────────────────────────────────────────────────
	// OnBecomeRelevant may fire before the controller is fully initialised.
	// Retry every tick until we succeed.
	if (!OwnerBTComp.IsValid())
	{
		OwnerBTComp = &OwnerComp;
	}

	if (!TrackedPerceptionComp.IsValid())
	{
		if (AAIController* AIC = OwnerComp.GetAIOwner())
		{
			if (TryBindPerceptionDelegates(AIC))
			{
				// Sync state immediately after the lazy bind succeeds — the player
				// may already be visible and we have missed the initial delegate call.
				SyncCurrentPerceptionState(OwnerComp);
			}
		}
	}

	// Nothing to expire yet.
	if (LastDetectionTime < 0.f)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World) { return; }

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) { return; }

	const float Now     = World->GetTimeSeconds();
	const float Elapsed = Now - LastDetectionTime;

	// ── 1. HasSeenPlayer cooldown expiry ──────────────────────────────────
	//
	// OnTargetPerceptionUpdated is EDGE-triggered: it fires when the player is
	// first detected and again when the stimulus expires/is forgotten, but it
	// does NOT re-fire while the target is CONTINUOUSLY inside the sight cone
	// (the perception component refreshes the stimulus age internally without
	// re-broadcasting the delegate).
	//
	// Without a poll here, HasSeenPlayer would expire every LostSightCooldown
	// seconds while the player stands still in front of the AI and never
	// recover until the player stepped out and back in.
	//
	// Fix: before clearing the flag, ask the perception component whether the
	// player is actually still in the currently-seen set.  If yes, refresh
	// LastDetectionTime so the cooldown resets naturally.  If not, the player
	// genuinely left the cone and we expire normally.
	if (bCachedHasSeenPlayer && Elapsed >= LostSightCooldown)
	{
		const float TimeBeforeSync = LastDetectionTime;
		SyncCurrentPerceptionState(OwnerComp);   // polls GetCurrentlyPerceivedActors

		if (LastDetectionTime <= TimeBeforeSync)
		{
			// SyncCurrentPerceptionState found nobody — player is genuinely gone.
			SetBBBool(BB, HasSeenPlayerKey, false, bCachedHasSeenPlayer);
			OnPlayerLost();
		}
		// else: a currently-seen player pawn was found and OnSightStimulus
		// freshened LastDetectionTime — cooldown window resets naturally.
	}

	// ── 2. HasHeardPlayer cooldown expiry ─────────────────────────────────
	if (bCachedHasHeardPlayer && LastHearTime >= 0.f)
	{
		if ((Now - LastHearTime) >= LostHearingCooldown)
		{
			SetBBBool(BB, HasHeardPlayerKey, false, bCachedHasHeardPlayer);
			LastHearTime = -1.f;
		}
	}

	// ── 3. LastKnownLocation + TargetEnemy retention expiry ───────────────
	//
	// Intentionally longer than the HasSeenPlayer cooldown so the AI can
	// navigate to the last known position after the "actively tracking" flag
	// has already cleared.
	if (bLocationIsSet && Elapsed >= LastKnownLocationRetention)
	{
		if (LastKnownLocationKey.IsSet()) { BB->ClearValue(LastKnownLocationKey.SelectedKeyName); }
		if (TargetEnemyKey.IsSet())       { BB->ClearValue(TargetEnemyKey.SelectedKeyName); }
		TrackedTargetPawn = nullptr;
		bLocationIsSet    = false;
		LastDetectionTime = -1.f;   // Reset master timer once all keys have been cleared.
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

bool UBTService_PlayerPerception::TryBindPerceptionDelegates(AAIController* AIC)
{
	if (!AIC) { return false; }

	UAIPerceptionComponent* PerceptionComp = AIC->GetPerceptionComponent();
	if (!PerceptionComp) { return false; }

	// Already bound to this component — nothing to do.  Return false so callers
	// don't treat this as a fresh bind and skip redundant SyncCurrentPerceptionState.
	if (TrackedPerceptionComp.Get() == PerceptionComp) { return false; }

	UnbindPerceptionDelegates();   // Detach from any previous component (safety).

	TrackedPerceptionComp = PerceptionComp;
	PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(
		this, &UBTService_PlayerPerception::HandlePerceptionUpdated);

	return true;  // Fresh bind — caller should sync current perception state.
}

void UBTService_PlayerPerception::SyncCurrentPerceptionState(UBehaviorTreeComponent& OwnerComp)
{
	UAIPerceptionComponent* PerceptionComp = TrackedPerceptionComp.Get();
	if (!PerceptionComp) { return; }

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) { return; }

	// Query actors that are currently being perceived through sight.
	// This handles the case where the player was already inside the sight cone
	// when the service started — OnTargetPerceptionUpdated won't fire retroactively
	// for detections that were established before our delegate was bound.
	TArray<AActor*> CurrentlySeen;
	PerceptionComp->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), CurrentlySeen);

	for (AActor* Actor : CurrentlySeen)
	{
		APawn* Pawn = Cast<APawn>(Actor);
		if (bOnlyDetectPlayers && (!Pawn || !Pawn->IsPlayerControlled())) { continue; }

		// Use the pawn's live location — no FAIStimulus is available here.
		OnSightStimulus(Pawn, Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector, BB);
	}
}

void UBTService_PlayerPerception::UnbindPerceptionDelegates()
{
	if (UAIPerceptionComponent* PerceptionComp = TrackedPerceptionComp.Get())
	{
		PerceptionComp->OnTargetPerceptionUpdated.RemoveDynamic(
			this, &UBTService_PlayerPerception::HandlePerceptionUpdated);
	}
	TrackedPerceptionComp = nullptr;
}

void UBTService_PlayerPerception::SetBBBool(
	UBlackboardComponent* BB,
	const FBlackboardKeySelector& Key,
	bool bNewValue,
	bool& bCachedValue) const
{
	if (!BB || !Key.IsSet() || bNewValue == bCachedValue) { return; }
	bCachedValue = bNewValue;
	BB->SetValueAsBool(Key.SelectedKeyName, bNewValue);
}

// ─────────────────────────────────────────────────────────────────────────────
// Description
// ─────────────────────────────────────────────────────────────────────────────

FString UBTService_PlayerPerception::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("Sight cooldown: %.1fs  |  Location retention: %.1fs\n"
		     "Hearing cooldown: %.1fs  |  Loudness threshold: %.0f%%\n"
		     "Only detect players: %s  |  Only damage by players: %s"),
		LostSightCooldown,
		LastKnownLocationRetention,
		LostHearingCooldown,
		HearingLoudnessThreshold * 100.f,
		bOnlyDetectPlayers   ? TEXT("Yes") : TEXT("No"),
		bOnlyDamageByPlayers ? TEXT("Yes") : TEXT("No"));
}

