// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h" // FOverlapResult
#include "InteractableIconSelectorComponent.generated.h"

UENUM(BlueprintType)
enum class EInteractableIconPositionScoreMode : uint8
{
	/** Usa projeção em tela e favorece o que está mais perto do centro do viewport. */
	ScreenCenter UMETA(DisplayName = "Screen Center"),

	/** Usa o Forward Vector do Pawn (ou Owner) e favorece o que está mais à frente do Pawn. */
	PawnForward UMETA(DisplayName = "Pawn Forward")
};

class AActor;
/**
 * Periodically selects the best interactable (by front/center/visibility score)
 * and tells it to show its icon while hiding all other nearby icons.
 *
 * Intended to run only for the locally controlled player.
 */
UCLASS(ClassGroup = (Interaction), meta = (BlueprintSpawnableComponent))
class LYRAGAME_API UInteractableIconSelectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractableIconSelectorComponent();
/** Enable/disable scanning + icon updates. */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Icon")
		void SetSelectorEnabled(bool bEnabled);
/** Returns the currently selected (best) actor, if any. */
	UFUNCTION(BlueprintPure, Category = "Interaction|Icon")
		AActor* GetCurrentBestActor() const { return CurrentBestActor.Get(); }
/** Force an immediate scan (useful after possession, camera mode changes, etc.). */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Icon")
		void ForceScan();
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	void StartTimerIfNeeded();
	void StopTimer();
	void ScanAndApply();
	bool IsLocallyControlledOwner() const;
	FVector GetViewLocation() const;
	FRotator GetViewRotation() const;
	bool QueryNearbyCandidates(TArray<AActor*>& OutCandidates) const;
	float ScoreCandidate(AActor* Candidate, const FVector& ViewLocation, const FVector& ViewForward, const FVector2D& ViewportCenter, const FVector2D& ViewportSize, bool& bOutVisible) const;
	bool HasLineOfSightTo(AActor* Candidate, const FVector& ViewLocation, const FVector& TargetLocation) const;
	FVector GetCandidateIconLocation(AActor* Candidate) const;
	void ApplySelection(AActor* NewBestActor, const TArray<AActor*>& ConsideredActors);
private:
// --- Tuning ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icon|Scan", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
		float ScanInterval = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icon|Scan", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
		float MaxScanDistance = 5000.0f;
/** Max number of nearby candidates to score each scan (performance guard). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icon|Scan", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
		int32 MaxCandidatesToScore = 32;
/** Only consider candidates roughly in front of the camera (dot threshold). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icon|Filter", meta = (AllowPrivateAccess = "true", ClampMin = "-1.0", ClampMax = "1.0"))
		float MinForwardDotToConsider = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icon|Visibility", meta = (AllowPrivateAccess = "true"))
		bool bRequireLineOfSight = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icon|Visibility", meta = (AllowPrivateAccess = "true"))
		TEnumAsByte<ECollisionChannel> LineOfSightTraceChannel = ECC_Visibility;
// --- Scoring Weights ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icon|Scoring", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
		float WeightCenter = 0.7f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icon|Scoring", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
		float WeightForward = 0.3f;
/** Extra multiplier if visible (only applied when bRequireLineOfSight==false). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icon|Scoring", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
		float VisibleScoreMultiplier = 1.25f;
/** Helps reduce flicker: only switch if new score > old score + delta. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icon|UX", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
		float MinSwitchScoreDelta = 0.05f;
// --- State ---
	UPROPERTY(Transient)
		bool bSelectorEnabled = true;
	FTimerHandle ScanTimerHandle;
	TWeakObjectPtr<AActor> CurrentBestActor;
	float CurrentBestScore = -1.0f;
// Reused arrays to avoid allocations.
	// NOTE: Avoid storing FOverlapResult as a member to prevent incomplete-type issues in certain build configurations.
	mutable TArray<TWeakObjectPtr<AActor>> CandidatesScratch;

	/** If true, requires the actor to implement the Blueprint Interface BPI_Interactable (checked via reflection). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icon|BPI", meta = (AllowPrivateAccess = "true"))
		bool bRequireBPIInteractable = true;

	/** Name of the BPI_Interactable function used as a presence check (any function on the interface works). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icon|BPI", meta = (AllowPrivateAccess = "true"))
		FName BPI_InteractablePresenceFuncName = TEXT("GetMinimumDistanceToShowIcon");

	/** Name of the BPI function that returns the minimum distance (float) to show the icon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icon|BPI", meta = (AllowPrivateAccess = "true"))
		FName BPI_GetMinimumDistanceFuncName = TEXT("GetMinimumDistanceToShowIcon");

	/** Optional: name of the BPI function that returns the icon world location (FVector). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icon|BPI", meta = (AllowPrivateAccess = "true"))
		FName BPI_GetIconWorldLocationFuncName = TEXT("GetIconWorldLocation");

	/** Interactable component class to look for on candidates (BPAC_Interactable). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icon|Component", meta = (AllowPrivateAccess = "true"))
		TSoftClassPtr<UActorComponent> InteractableComponentClass;

	/** If true and InteractableComponentClass isn't set/loaded, accept any actor that has a component with SetWidgetVisibility(bool,float). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icon|Component", meta = (AllowPrivateAccess = "true"))
		bool bAllowAnyComponentWithSetWidgetVisibility = true;

	bool DoesActorImplementBPI(AActor* Actor) const;
	float GetMinimumDistanceViaBPI(AActor* Actor) const;
	bool GetIconWorldLocationViaBPI(AActor* Actor, FVector& OutLocation) const;

	UActorComponent* FindInteractableComponent(AActor* Actor) const;
	void SetWidgetVisibilityOnActor(AActor* Actor, bool bVisible, float PlayerDistance) const;

	bool IsInteractableActor(AActor* Actor) const;
	float GetMinimumDistanceToShow(AActor* Actor) const;
	FVector GetIconWorldLocationFromActor(AActor* Actor) const;

	/** Actors that were considered in the previous scan. Used to hide icons when they drop out of range/filters. */
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AActor>> PreviouslyConsideredActors;

	// --- Debug ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icon|Debug", meta = (AllowPrivateAccess = "true"))
	bool bDebugScoring = false;

	/** If true and bDebugScoring is enabled, draws debug strings at candidate locations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icon|Debug", meta = (AllowPrivateAccess = "true"))
	bool bDebugDraw = false;

	/** Max candidates to log per scan (prevents log spam). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icon|Debug", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 DebugMaxCandidatesToLog = 10;

	/** Duration for debug draw strings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icon|Debug", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DebugDrawDuration = 0.15f;

	/**
	 * When ScoreCandidate returns < 0, this stores the reason for the last evaluated candidate.
	 * Only valid within the current scan and only used for debug.
	 */
	mutable FString LastRejectReason;

	/** Define como o score de posicionamento deve ser calculado (centro da tela ou forward do pawn). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icon|Scoring", meta = (AllowPrivateAccess = "true"))
	EInteractableIconPositionScoreMode PositionScoreMode = EInteractableIconPositionScoreMode::ScreenCenter;

	/**
	 * Se true, mesmo no modo PawnForward o candidato precisa estar on-screen (projeta e rejeita se estiver fora).
	 * Útil para evitar selecionar interagíveis atrás/frente mas fora do FOV/viewport.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icon|Scoring", meta = (AllowPrivateAccess = "true"))
	bool bRejectOffScreenInPawnForwardMode = true;

	/**
	 * No modo ScreenCenter, rejeita candidatos cuja projeção esteja muito longe do centro do viewport.
	 * Valor normalizado (0..1) baseado na meia-diagonal do viewport: 0 = exatamente no centro, 1 = no limite da meia-diagonal.
	 * Ex: 0.25 tende a exigir que o alvo permaneça relativamente próximo ao centro pra manter o ícone.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Icon|Scoring", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float ScreenCenterMaxNormalizedDistance = 0.35f;
};
