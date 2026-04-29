// Copyright MyShooterScenarios. All Rights Reserved.

#include "AI/BTService_PeekWillingness.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

UBTService_PeekWillingness::UBTService_PeekWillingness(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = "Peek Willingness (MYST)";
	bNotifyTick = true;

	// This service is a pure aggregator — it only reads BB values written by the
	// two upstream services, so a slightly longer interval is fine.
	Interval = 0.20f;
	RandomDeviation = 0.05f;

	// Restrict each key selector to its expected type.
	HealthPercentageKey.AddFloatFilter(this,       GET_MEMBER_NAME_CHECKED(UBTService_PeekWillingness, HealthPercentageKey));
	IsIsolatedKey.AddBoolFilter(this,              GET_MEMBER_NAME_CHECKED(UBTService_PeekWillingness, IsIsolatedKey));
	HasTargetInCoverKey.AddBoolFilter(this,        GET_MEMBER_NAME_CHECKED(UBTService_PeekWillingness, HasTargetInCoverKey));
	TargetTimeInCoverKey.AddFloatFilter(this,      GET_MEMBER_NAME_CHECKED(UBTService_PeekWillingness, TargetTimeInCoverKey));
	IsTargetEngagingOtherKey.AddBoolFilter(this,   GET_MEMBER_NAME_CHECKED(UBTService_PeekWillingness, IsTargetEngagingOtherKey));
	OutOfAmmoKey.AddBoolFilter(this,               GET_MEMBER_NAME_CHECKED(UBTService_PeekWillingness, OutOfAmmoKey));
	HasTakenDamageRecentlyKey.AddBoolFilter(this,  GET_MEMBER_NAME_CHECKED(UBTService_PeekWillingness, HasTakenDamageRecentlyKey));
	TargetIsReloadingKey.AddBoolFilter(this,       GET_MEMBER_NAME_CHECKED(UBTService_PeekWillingness, TargetIsReloadingKey));
	TargetIsLowHealthKey.AddBoolFilter(this,       GET_MEMBER_NAME_CHECKED(UBTService_PeekWillingness, TargetIsLowHealthKey));
	PeekWillingnessScoreKey.AddFloatFilter(this,   GET_MEMBER_NAME_CHECKED(UBTService_PeekWillingness, PeekWillingnessScoreKey));
}

// ─────────────────────────────────────────────────────────────────────────────
// Asset initialisation — resolves SelectedKeyID for every key selector
// ─────────────────────────────────────────────────────────────────────────────

void UBTService_PeekWillingness::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		HealthPercentageKey.ResolveSelectedKey(*BBAsset);
		IsIsolatedKey.ResolveSelectedKey(*BBAsset);
		HasTargetInCoverKey.ResolveSelectedKey(*BBAsset);
		TargetTimeInCoverKey.ResolveSelectedKey(*BBAsset);
		IsTargetEngagingOtherKey.ResolveSelectedKey(*BBAsset);
		OutOfAmmoKey.ResolveSelectedKey(*BBAsset);
		HasTakenDamageRecentlyKey.ResolveSelectedKey(*BBAsset);
		TargetIsReloadingKey.ResolveSelectedKey(*BBAsset);
		TargetIsLowHealthKey.ResolveSelectedKey(*BBAsset);
		PeekWillingnessScoreKey.ResolveSelectedKey(*BBAsset);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Instance memory
// ─────────────────────────────────────────────────────────────────────────────

uint16 UBTService_PeekWillingness::GetInstanceMemorySize() const
{
	return sizeof(FPeekWillingnessMemory);
}

void UBTService_PeekWillingness::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	new (NodeMemory) FPeekWillingnessMemory();
}

// ─────────────────────────────────────────────────────────────────────────────
// Tick
// ─────────────────────────────────────────────────────────────────────────────

void UBTService_PeekWillingness::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	FPeekWillingnessMemory* Memory = reinterpret_cast<FPeekWillingnessMemory*>(NodeMemory);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!Memory || !BB)
	{
		return;
	}

	float Score = 0.f;

	// ─── Offensive bonuses — player is vulnerable ─────────────────────────────

	// +TargetIsReloadingBonus: target ASC has active reload tag (golden window)
	if (TargetIsReloadingKey.IsSet() && BB->GetValueAsBool(TargetIsReloadingKey.SelectedKeyName))
	{
		Score += TargetIsReloadingBonus;
	}

	// +TargetExposedBonus: target is NOT in cover
	if (HasTargetInCoverKey.IsSet() && !BB->GetValueAsBool(HasTargetInCoverKey.SelectedKeyName))
	{
		Score += TargetExposedBonus;
	}

	// +TargetLowHealthBonus: target health below threshold
	if (TargetIsLowHealthKey.IsSet() && BB->GetValueAsBool(TargetIsLowHealthKey.SelectedKeyName))
	{
		Score += TargetLowHealthBonus;
	}

	// +TargetDistractedBonus: target is engaging a different AI
	if (IsTargetEngagingOtherKey.IsSet() && BB->GetValueAsBool(IsTargetEngagingOtherKey.SelectedKeyName))
	{
		Score += TargetDistractedBonus;
	}

	// +TargetOverduePeekBonus: target has been in cover longer than the overdue threshold
	if (TargetTimeInCoverKey.IsSet())
	{
		const float TimeInCover = BB->GetValueAsFloat(TargetTimeInCoverKey.SelectedKeyName);
		if (TimeInCover >= TargetOverduePeekTime)
		{
			Score += TargetOverduePeekBonus;
		}
	}

	// ─── Defensive penalties — AI is unsafe ───────────────────────────────────

	// -OutOfAmmoPenalty: AI is actively reloading
	if (OutOfAmmoKey.IsSet() && BB->GetValueAsBool(OutOfAmmoKey.SelectedKeyName))
	{
		Score -= OutOfAmmoPenalty;
	}

	// -RecentlyHitPenalty: AI took significant damage recently
	if (HasTakenDamageRecentlyKey.IsSet() && BB->GetValueAsBool(HasTakenDamageRecentlyKey.SelectedKeyName))
	{
		Score -= RecentlyHitPenalty;
	}

	// -LowSelfHealthPenalty: AI's own health is critically low
	if (HealthPercentageKey.IsSet())
	{
		const float SelfHealthPct = BB->GetValueAsFloat(HealthPercentageKey.SelectedKeyName);
		if (SelfHealthPct < LowSelfHealthThreshold)
		{
			Score -= LowSelfHealthPenalty;
		}
	}

	// -IsolatedPenalty: no allies nearby
	if (IsIsolatedKey.IsSet() && BB->GetValueAsBool(IsIsolatedKey.SelectedKeyName))
	{
		Score -= IsolatedPenalty;
	}

	// ─── Clamp and write ──────────────────────────────────────────────────────
	//
	// Dead-band guard: only write to the BB when the score has changed by more than
	// ScoreWriteDeadBand.  The score is an arithmetic sum of several input floats;
	// even tiny changes in HealthPct or TargetCoverTime produce a different float
	// result every tick, which would fire a BB change notification at the service's
	// full tick rate.  Any parent-level BB Float Decorator with Observer Aborts
	// watching this key would then abort the active task (e.g. EQS_FindCover) before
	// it can finish, causing the BT to loop on the first child indefinitely.
	const float ClampedScore = FMath::Clamp(Score, 0.f, 1.f);

	if (PeekWillingnessScoreKey.IsSet() && Memory)
	{
		if (FMath::Abs(ClampedScore - Memory->LastWrittenScore) >= ScoreWriteDeadBand)
		{
			Memory->LastWrittenScore = ClampedScore;
			BB->SetValueAsFloat(PeekWillingnessScoreKey.SelectedKeyName, ClampedScore);
		}
	}

	// ─── Blueprint threshold transition event ─────────────────────────────────

	const bool bIsNowReady = ClampedScore >= PeekThreshold;
	if (bIsNowReady != Memory->bWasReady)
	{
		OnPeekReadinessChanged(bIsNowReady);
		Memory->bWasReady = bIsNowReady;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Editor description
// ─────────────────────────────────────────────────────────────────────────────

FString UBTService_PeekWillingness::GetStaticDescription() const
{
	// Offensive summary
	const FString Offensive = FString::Printf(
		TEXT("+ Target reloading: %.2f | exposed: %.2f | low HP: %.2f\n"
			 "+ Distracted: %.2f | overdue cover (>%.0fs): %.2f"),
		TargetIsReloadingBonus, TargetExposedBonus, TargetLowHealthBonus,
		TargetDistractedBonus, TargetOverduePeekTime, TargetOverduePeekBonus
	);

	// Defensive summary
	const FString Defensive = FString::Printf(
		TEXT("- Out of ammo: %.2f | recently hit: %.2f\n"
			 "- Low self HP (<%.0f%%): %.2f | isolated: %.2f"),
		OutOfAmmoPenalty, RecentlyHitPenalty,
		LowSelfHealthThreshold * 100.f, LowSelfHealthPenalty,
		IsolatedPenalty
	);

	return FString::Printf(TEXT("Peek threshold: %.2f\n%s\n%s"),
		PeekThreshold, *Offensive, *Defensive);
}


