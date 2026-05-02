// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "BehaviorTree/BTService.h"
#include "Perception/AIPerceptionTypes.h"

#include "BTService_PlayerPerception.generated.h"

class UAIPerceptionComponent;

/**
 * BT Service: Player Perception (MYST)
 *
 * Binds to a UAIPerceptionComponent on the owning AIController and writes the
 * following Blackboard keys (all names configurable on the BT node):
 *
 *  ┌────────────────────────────────┬──────────────────────────────────────────────────────────┐
 *  │ Blackboard Key (suggested)     │ What it tracks                                           │
 *  ├────────────────────────────────┼──────────────────────────────────────────────────────────┤
 *  │ HasSeenPlayer        (Bool)    │ Instant-true on sight; stays true for LostSightCooldown  │
 *  │                                │ seconds AFTER the player leaves the sensing cone.        │
 *  │ TargetEnemy          (Object)  │ Set to the detected player pawn on sight/damage; cleared │
 *  │                                │ at the same time as LastKnownLocation (long retention).  │
 *  │ LastKnownLocation    (Vector)  │ Set to the player's location on sight/hear, or to the    │
 *  │                                │ instigator's position on a damage event.  Cleared after  │
 *  │                                │ LastKnownLocationRetention seconds.                      │
 *  │ HasHeardPlayer       (Bool)    │ Instant-true on a qualifying noise; decays after          │
 *  │                                │ LostHearingCooldown seconds.                             │
 *  └────────────────────────────────┴──────────────────────────────────────────────────────────┘
 *
 * ── Setup ──────────────────────────────────────────────────────────────────
 *  1. Add UAIPerceptionComponent to the AI Controller Blueprint.
 *       - Add an AI Sight Config: set SightRadius, LoseSightRadius, PeripheralVisionAngle.
 *         In DetectionByAffiliation set DetectEnemies=true, DetectFriendlies=false.
 *       - Add an AI Hearing Config similarly.
 *       - Add an AI Damage Config (UAISenseConfig_Damage) — no affiliation filter needed.
 *  2. Ensure the player Character Blueprint has a UAIPerceptionStimuliSourceComponent
 *     (or enable bAutoRegisterAllPawnsAsSources on the AIPerceptionSystem settings).
 *  3. In your damage handling code (e.g. ULyraHealthComponent or a custom GA), call:
 *       UAISense_Damage::ReportDamageEvent(World, DamagedActor, Instigator, Amount,
 *                                          InstigatorLocation, HitLocation);
 *     This fires the damage stimulus and triggers OnTargetPerceptionUpdated on the AI.
 *  4. Place this service in the Behavior Tree and wire the BB key selectors.
 *  5. Tune LostSightCooldown and LastKnownLocationRetention on the node.
 *
 * ── Why UAIPerceptionComponent over UPawnSensingComponent ──────────────────
 *  - ALyraPlayerBotController already implements GetTeamAttitudeTowards, which
 *    UAIPerceptionComponent uses natively for affiliation filtering — hostile
 *    actors are filtered at the perception level with no extra code.
 *  - Proper UE5 perception system: per-sense configs, sense age expiry, and
 *    editor tooling (AIPerception debug channel, perception debug display).
 *  - Single OnTargetPerceptionUpdated delegate covers both sight and hearing.
 *
 * ── Pattern notes ──────────────────────────────────────────────────────────
 *  bCreateNodeInstance = true — each AI controller gets its own UObject instance
 *  so per-AI state is stored as plain member variables (same pattern as
 *  BTService_AIStateObserver).  OnBecomeRelevant binds delegates eagerly; TickNode
 *  retries the bind lazily if the perception component wasn't ready yet.
 *
 *  BB writes are gated on actual state changes to avoid spamming change
 *  notifications that trigger Observer-Abort decorators at tick rate.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Player Perception (MYST)"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API UBTService_PlayerPerception : public UBTService
{
	GENERATED_BODY()

public:

	UBTService_PlayerPerception(const FObjectInitializer& ObjectInitializer);

	virtual FString GetStaticDescription() const override;
	virtual void    InitializeFromAsset(UBehaviorTree& Asset) override;

	/**
	 * Safety net: unbind the perception delegate if GC collects this node
	 * instance before OnCeaseRelevant fires (e.g. forceful actor destroy or
	 * a PIE stop that bypasses the normal BT shutdown sequence).  Without this,
	 * UAIPerceptionComponent::OnTargetPerceptionUpdated retains a dangling
	 * FScriptDelegate whose bound UObject has already been destroyed.
	 */
	virtual void BeginDestroy() override;

protected:

	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// ── Blackboard Keys ────────────────────────────────────────────────────

	/**
	 * Bool key — instant-true when a hostile pawn enters the sight sense.
	 * Decays to false only after LostSightCooldown seconds with no new stimulus.
	 * Suggested BB key name: HasSeenPlayer
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception|Keys")
	FBlackboardKeySelector HasSeenPlayerKey;

	/**
	 * Object key — set to the detected player pawn on sight (alongside HasSeenPlayer).
	 * Cleared at the same time as LastKnownLocation (LastKnownLocationRetention cooldown),
	 * so BT branches can still reference the target actor while searching the last known area.
	 * Suggested BB key name: TargetEnemy
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception|Keys")
	FBlackboardKeySelector TargetEnemyKey;

	/**
	 * Vector key — set to the stimulus location on each sight/hear event.
	 * Remains set for LastKnownLocationRetention seconds after the last detection,
	 * then cleared so the AI can search the last known area and then give up.
	 * Suggested BB key name: LastKnownLocation
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception|Keys")
	FBlackboardKeySelector LastKnownLocationKey;

	/**
	 * Bool key — (Phase 2 / Hearing) instant-true when a qualifying noise is
	 * perceived.  Decays to false after LostHearingCooldown seconds.
	 * Leave unset (None) to disable hearing tracking.
	 * Suggested BB key name: HasHeardPlayer
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception|Keys")
	FBlackboardKeySelector HasHeardPlayerKey;

	// ── Sight Tuning ───────────────────────────────────────────────────────

	/**
	 * Seconds the player must remain out-of-sight before HasSeenPlayer resets
	 * to false.  The AI will keep tracking/chasing for this duration even after
	 * losing line-of-sight — give it a generous value (e.g. 5s) so behaviour
	 * does not flicker when the player steps briefly behind cover.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception|Sight",
		meta = (ClampMin = "0.1", UIMin = "1.0", UIMax = "30.0", ForceUnits = "s",
		        DisplayName = "Lost Sight Cooldown (s)"))
	float LostSightCooldown = 5.0f;

	/**
	 * Seconds after the last confirmed detection before LastKnownLocation and
	 * TargetEnemy are cleared.  Should be >= LostSightCooldown so the location
	 * outlives the HasSeenPlayer flag and the AI always has somewhere to search
	 * after losing the target.  Typical value: 15–30 s.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception|Sight",
		meta = (ClampMin = "0.1", UIMin = "5.0", UIMax = "60.0", ForceUnits = "s",
		        DisplayName = "Last Known Location Retention (s)"))
	float LastKnownLocationRetention = 15.0f;

	/**
	 * When true, the perception callback will only react to player-controlled pawns,
	 * providing a code-side safety net in addition to the perception component's
	 * affiliation filter (DetectEnemies=true on the sight config).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception|Sight")
	bool bOnlyDetectPlayers = true;

	// ── Hearing Tuning (Phase 2) ───────────────────────────────────────────

	/**
	 * Seconds after the last perceived noise before HasHeardPlayer resets to false.
	 * Ignored if HasHeardPlayerKey is not set.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception|Hearing",
		meta = (ClampMin = "0.1", UIMin = "1.0", UIMax = "30.0", ForceUnits = "s",
		        DisplayName = "Lost Hearing Cooldown (s)"))
	float LostHearingCooldown = 3.0f;

	/**
	 * Minimum stimulus strength (0–1) from a hearing event to count as a detection.
	 * Maps to FAIStimulus::Strength, which equals the loudness passed to
	 * UAISense_Hearing::ReportNoiseEvent.  Prevents faint ambient sounds from
	 * arming HasHeardPlayer.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception|Hearing",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HearingLoudnessThreshold = 0.5f;

	// ── Damage Tuning ─────────────────────────────────────────────────────

	/**
	 * When true, the damage callback only reacts to damage instigated by player-controlled
	 * pawns.  Prevents friendly-fire or environmental damage from setting LastKnownLocation.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception|Damage")
	bool bOnlyDamageByPlayers = true;

	// ── Blueprint Event Hooks ──────────────────────────────────────────────

	/** Fires when the AI first spots the player (false → true transition). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Perception|Events",
		meta = (DisplayName = "On Player First Spotted"))
	void OnPlayerFirstSpotted(APawn* SpottedPawn);

	/** Fires when HasSeenPlayer transitions true → false (lost-sight cooldown elapsed). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Perception|Events",
		meta = (DisplayName = "On Player Lost"))
	void OnPlayerLost();

	/** (Phase 2) Fires on each qualifying noise event. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Perception|Events",
		meta = (DisplayName = "On Noise Heard"))
	void OnNoiseHeard(APawn* NoiseMaker, const FVector& Location, float Loudness);

	/**
	 * Fires when the AI receives a damage stimulus.
	 * AttackerPawn may be null if the instigator was not a pawn.
	 * InstigatorLocation is the position recorded by UAISense_Damage — use it to
	 * navigate/investigate the incoming fire direction.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Perception|Events",
		meta = (DisplayName = "On Damage Taken"))
	void OnDamageTaken(APawn* AttackerPawn, const FVector& InstigatorLocation);

private:

	// ── Delegate callback ─────────────────────────────────────────────────
	// Single entry-point for all senses.  Must be UFUNCTION for dynamic multicast.

	UFUNCTION()
	void HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	// ── Per-sense helpers (called from HandlePerceptionUpdated) ───────────

	void OnSightStimulus(APawn* SeenPawn, const FVector& Location, UBlackboardComponent* BB);
	void OnHearingStimulus(APawn* NoiseMaker, const FVector& Location, float Loudness, UBlackboardComponent* BB);
	void OnDamageStimulus(APawn* AttackerPawn, const FVector& InstigatorLocation, UBlackboardComponent* BB);

	// ── Internal helpers ──────────────────────────────────────────────────

	/** Finds and binds the perception delegate on the AI controller. Returns true on a FRESH bind. */
	bool TryBindPerceptionDelegates(AAIController* AIC);

	/** Removes our callback from the tracked perception component. Safe to call when nothing is bound. */
	void UnbindPerceptionDelegates();

	/**
	 * Queries the perception component for actors that are ALREADY perceived and
	 * pushes their current state into the Blackboard.
	 *
	 * OnTargetPerceptionUpdated only fires on state CHANGES, so any detection that
	 * happened before our delegate was bound (e.g. the player was already in the
	 * sight cone when the service became active) would be silently missed without
	 * this initial sync.  Called immediately after every successful bind.
	 */
	void SyncCurrentPerceptionState(UBehaviorTreeComponent& OwnerComp);

	/** Writes a bool BB key only when the value actually changes (avoids spurious BB notifications). */
	void SetBBBool(UBlackboardComponent* BB, const FBlackboardKeySelector& Key, bool bNewValue, bool& bCachedValue) const;

	// ── Per-AI instance state (valid because bCreateNodeInstance = true) ──

	/** World-time of the last successfully sensed stimulus that passed all filters. -1 = inactive. */
	float LastDetectionTime = -1.f;

	/** World-time of the last qualifying hearing stimulus. -1 = inactive. */
	float LastHearTime = -1.f;

	/** Cached HasSeenPlayer value — used for write-only-on-change optimisation. */
	bool bCachedHasSeenPlayer = false;

	/** Cached HasHeardPlayer value — used for write-only-on-change optimisation. */
	bool bCachedHasHeardPlayer = false;


	/** True once a detection event has set LastKnownLocation (tracks whether a clear is pending). */
	bool bLocationIsSet = false;

	/** The last pawn we detected — kept so TargetEnemy can be cleared on retention expiry. */
	TWeakObjectPtr<APawn> TrackedTargetPawn;

	/** Weak ref to the perception component we've bound to. Used for unbinding and lazy-bind guard. */
	TWeakObjectPtr<UAIPerceptionComponent> TrackedPerceptionComp;

	/** Weak ref to our BT component — needed to write the BB from inside the async delegate. */
	TWeakObjectPtr<UBehaviorTreeComponent> OwnerBTComp;
};

