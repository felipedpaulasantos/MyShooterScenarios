// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "Components/ControllerComponent.h"
#include "Inventory/LyraInventoryItemInstance.h"

#include "MYSTWeaponBarComponent.generated.h"

class ULyraEquipmentInstance;
class ULyraEquipmentManagerComponent;

// ─────────────────────────────────────────────────────────────────────────────
// Message structs broadcast via UGameplayMessageSubsystem
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Broadcast on MYST.WeaponBar.Message.SlotsChanged whenever the fixed or pickup
 * slot contents change.  HUD widgets subscribe to this to refresh the weapon bar.
 */
USTRUCT(BlueprintType)
struct FMYSTWeaponBarSlotsChangedMessage
{
	GENERATED_BODY()

	/** The PlayerController that owns this weapon bar. */
	UPROPERTY(BlueprintReadOnly, Category="WeaponBar")
	TObjectPtr<AActor> Owner = nullptr;

	/** All fixed weapon slots (null entries = empty slot). */
	UPROPERTY(BlueprintReadOnly, Category="WeaponBar")
	TArray<TObjectPtr<ULyraInventoryItemInstance>> FixedSlots;

	/** The pickup weapon slot (null if empty). */
	UPROPERTY(BlueprintReadOnly, Category="WeaponBar")
	TObjectPtr<ULyraInventoryItemInstance> PickupSlot = nullptr;
};

/**
 * Broadcast on MYST.WeaponBar.Message.ActiveSlotChanged whenever the player
 * switches weapons.  HUD widgets subscribe to this to highlight the active slot.
 *
 * ActiveSlotIndex values:
 *   UMYSTWeaponBarComponent::PickupSlotActiveIndex (-2) → pickup slot is active
 *   INDEX_NONE (-1)                                     → nothing equipped
 *   0 or greater                                        → fixed slot index
 */
USTRUCT(BlueprintType)
struct FMYSTWeaponBarActiveSlotChangedMessage
{
	GENERATED_BODY()

	/** The PlayerController that owns this weapon bar. */
	UPROPERTY(BlueprintReadOnly, Category="WeaponBar")
	TObjectPtr<AActor> Owner = nullptr;

	/**
	 * Active slot index.
	 *   -2  = pickup slot active  (UMYSTWeaponBarComponent::PickupSlotActiveIndex)
	 *   -1  = nothing equipped    (INDEX_NONE)
	 *    0+ = fixed slot index
	 */
	UPROPERTY(BlueprintReadOnly, Category="WeaponBar")
	int32 ActiveSlotIndex = INDEX_NONE;

	/** The item currently equipped, or null if nothing is active. */
	UPROPERTY(BlueprintReadOnly, Category="WeaponBar")
	TObjectPtr<ULyraInventoryItemInstance> ActiveWeapon = nullptr;
};

// ─────────────────────────────────────────────────────────────────────────────
// UMYSTWeaponBarComponent
// ─────────────────────────────────────────────────────────────────────────────

/**
 * UMYSTWeaponBarComponent
 *
 * ControllerComponent that manages the player's weapon slots, separating
 * permanent "Fixed" weapons from the single temporary "Pickup" weapon.
 *
 * Add to the PlayerController via UGameFeatureAction_AddComponents in the Experience.
 *
 * ── SLOT TYPES ───────────────────────────────────────────────────────────────
 * Fixed slots   Persistent weapon slots.  Count is initialised at BeginPlay from
 *               UMYSTInventoryAttributeSet::MaxWeaponSlots (default = 2).
 *               Resize by calling ResizeFixedSlots() after applying a capacity GE.
 *
 * Pickup slot   Single slot for a temporary world-pickup weapon.
 *               Call ClearPickupWeapon() when ammo depletes or the player discards
 *               the weapon.  Falls back to the first occupied fixed slot.
 *
 * ── ACTIVE SLOT ──────────────────────────────────────────────────────────────
 * SetActiveFixedSlot(Index) / ActivatePickupSlot() equip the selected weapon via
 * ULyraEquipmentManagerComponent on the owned pawn.  Only one weapon is equipped
 * at any time.
 *
 * ── HUD MESSAGING ────────────────────────────────────────────────────────────
 * Slot contents → MYST.WeaponBar.Message.SlotsChanged     (FMYSTWeaponBarSlotsChangedMessage)
 * Active slot   → MYST.WeaponBar.Message.ActiveSlotChanged (FMYSTWeaponBarActiveSlotChangedMessage)
 */
UCLASS(Blueprintable, ClassGroup=Inventory, meta=(BlueprintSpawnableComponent,
       DisplayName="MYST Weapon Bar Component"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API UMYSTWeaponBarComponent : public UControllerComponent
{
	GENERATED_BODY()

public:

	UMYSTWeaponBarComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~End of UActorComponent interface

	// ─── Sentinel ─────────────────────────────────────────────────────────────

	/**
	 * Sentinel value returned by GetActiveSlotIndex() when the pickup weapon is active.
	 * Also usable from Blueprint via GetPickupSlotActiveIndex().
	 */
	static constexpr int32 PickupSlotActiveIndex = -2;

	/** Blueprint-accessible version of the PickupSlotActiveIndex sentinel (-2). */
	UFUNCTION(BlueprintPure, Category="MYST|WeaponBar",
	          meta=(CompactNodeTitle="Pickup Active Index"))
	static int32 GetPickupSlotActiveIndex() { return PickupSlotActiveIndex; }

	// ─── Fixed slot management ─────────────────────────────────────────────────

	/**
	 * Places an item in a fixed weapon slot.  Authority-only.
	 * If the slot is occupied the existing item is displaced (not removed from inventory).
	 * If this slot is currently active the weapon is re-equipped.
	 *
	 * @param SlotIndex  0-based fixed slot index. Must be in range [0, NumFixedSlots).
	 * @param Item       The inventory item to place; must not be null.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="MYST|WeaponBar")
	void AddWeaponToFixedSlot(int32 SlotIndex, ULyraInventoryItemInstance* Item);

	/**
	 * Removes and returns the item from a fixed weapon slot.  Authority-only.
	 * If the slot was active, the weapon is unequipped and the active index is cleared.
	 *
	 * @param SlotIndex  0-based fixed slot index.
	 * @return           The removed item, or null if the slot was already empty.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="MYST|WeaponBar")
	ULyraInventoryItemInstance* RemoveWeaponFromFixedSlot(int32 SlotIndex);

	/** Returns a snapshot of the fixed slots array (may contain null entries for empty slots). */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="MYST|WeaponBar")
	TArray<ULyraInventoryItemInstance*> GetFixedSlots() const;

	/** Number of fixed weapon slots (driven by UMYSTInventoryAttributeSet::MaxWeaponSlots). */
	UFUNCTION(BlueprintPure, Category="MYST|WeaponBar")
	int32 GetNumFixedSlots() const { return FixedSlots.Num(); }

	/**
	 * Resizes the fixed slots array.  Call this after applying a capacity-upgrade GE.
	 * Authority-only.  Growing adds empty slots.  Shrinking preserves the lowest indices;
	 * weapons in removed slots are displaced but remain in the inventory.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="MYST|WeaponBar")
	void ResizeFixedSlots(int32 NewCount);

	/**
	 * Returns the lowest-index empty fixed slot, or INDEX_NONE if all slots are occupied.
	 * Useful when auto-assigning a newly picked-up fixed weapon.
	 */
	UFUNCTION(BlueprintPure, Category="MYST|WeaponBar")
	int32 GetNextFreeFixedSlot() const;

	// ─── Pickup slot management ───────────────────────────────────────────────

	/**
	 * Puts an item in the pickup weapon slot.  Authority-only.
	 * If a pickup weapon is already held it is silently displaced (not removed from inventory).
	 * If the pickup slot was active the new item is immediately equipped.
	 *
	 * Passing null is equivalent to calling ClearPickupWeapon().
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="MYST|WeaponBar")
	void SetPickupWeapon(ULyraInventoryItemInstance* Item);

	/**
	 * Empties the pickup weapon slot.  Authority-only.
	 * Intended to be called when the weapon's ammo reaches zero.
	 * If the pickup weapon was active, falls back to the first occupied fixed slot,
	 * or fully deactivates if no fixed slot is available.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="MYST|WeaponBar")
	void ClearPickupWeapon();

	/** Returns the current pickup weapon item, or null if the slot is empty. */
	UFUNCTION(BlueprintPure, Category="MYST|WeaponBar")
	ULyraInventoryItemInstance* GetPickupWeapon() const { return PickupSlot; }

	/** Returns true when the pickup slot holds an item. */
	UFUNCTION(BlueprintPure, Category="MYST|WeaponBar")
	bool HasPickupWeapon() const { return PickupSlot != nullptr; }

	// ─── Active slot selection ─────────────────────────────────────────────────

	/**
	 * Activates a fixed weapon slot, equipping the weapon via EquipmentManagerComponent.
	 * Server-RPC: safe to call from the client.
	 * Ignored if the index is out-of-range or the slot is empty.
	 *
	 * @param SlotIndex  0-based fixed slot index.
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="MYST|WeaponBar")
	void SetActiveFixedSlot(int32 SlotIndex);

	/**
	 * Activates the pickup weapon slot, equipping it via EquipmentManagerComponent.
	 * Server-RPC: safe to call from the client.
	 * Ignored if the pickup slot is empty.
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="MYST|WeaponBar")
	void ActivatePickupSlot();

	/**
	 * Unequips the current weapon without selecting another (active index → INDEX_NONE).
	 * Server-RPC: safe to call from the client.
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="MYST|WeaponBar")
	void ClearActiveSlot();

	/**
	 * Cycles to the next non-empty weapon slot (fixed 0…N, then pickup).
	 * Wraps around.  Calls SetActiveFixedSlot / ActivatePickupSlot internally (server RPCs).
	 */
	UFUNCTION(BlueprintCallable, Category="MYST|WeaponBar")
	void CycleWeaponForward();

	/**
	 * Cycles to the previous non-empty weapon slot.  Wraps around.
	 * Calls SetActiveFixedSlot / ActivatePickupSlot internally (server RPCs).
	 */
	UFUNCTION(BlueprintCallable, Category="MYST|WeaponBar")
	void CycleWeaponBackward();

	// ─── Queries ──────────────────────────────────────────────────────────────

	/** Returns the currently active weapon item instance, or null if nothing is equipped. */
	UFUNCTION(BlueprintPure, Category="MYST|WeaponBar")
	ULyraInventoryItemInstance* GetActiveWeapon() const;

	/**
	 * Returns the active slot index:
	 *   PickupSlotActiveIndex (-2) → pickup slot
	 *   INDEX_NONE            (-1) → nothing equipped
	 *   0 or greater               → fixed slot index
	 */
	UFUNCTION(BlueprintPure, Category="MYST|WeaponBar")
	int32 GetActiveSlotIndex() const { return ActiveSlotIndex; }

	/** Returns true when the pickup weapon is the currently active weapon. */
	UFUNCTION(BlueprintPure, Category="MYST|WeaponBar")
	bool IsPickupWeaponActive() const { return ActiveSlotIndex == PickupSlotActiveIndex; }

	/**
	 * Returns the ULyraEquipmentInstance currently managed by this bar (the live spawned
	 * equipment object), or null when nothing is equipped or on the client.
	 *
	 * Use this to reach the spawned world actors (GetSpawnedActors()), pawn, or
	 * equipment-specific data from Blueprint.
	 *
	 * Only valid on the server / listen-server authority.  On pure clients the
	 * equipment instance state is owned by ULyraEquipmentManagerComponent.
	 */
	UFUNCTION(BlueprintPure, Category="MYST|WeaponBar",
	          DisplayName="Get Current Equipment Instance")
	ULyraEquipmentInstance* GetCurrentEquipmentInstance() const { return EquippedItem; }

	// ─── Blueprint implementable events ───────────────────────────────────────

	/**
	 * Called on both server and clients whenever the fixed or pickup slot contents change.
	 * Override in a Blueprint subclass to update the HUD or react to weapon changes
	 * without subscribing to GameplayMessages.
	 *
	 * @param NewFixedSlots   Snapshot of all fixed slots (may contain null entries).
	 * @param NewPickupSlot   The pickup weapon item, or null if empty.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="MYST|WeaponBar",
	          DisplayName="On Slots Changed")
	void K2_OnSlotsChanged(const TArray<ULyraInventoryItemInstance*>& NewFixedSlots,
	                        ULyraInventoryItemInstance* NewPickupSlot);

	/**
	 * Called on both server and clients whenever the active weapon slot changes.
	 * Override in a Blueprint subclass to highlight the new active slot in the HUD.
	 *
	 * @param NewActiveSlotIndex  -2 = pickup active, -1 = nothing, 0+ = fixed slot.
	 * @param NewActiveWeapon     The newly active weapon item, or null if deactivated.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="MYST|WeaponBar",
	          DisplayName="On Active Slot Changed")
	void K2_OnActiveSlotChanged(int32 NewActiveSlotIndex,
	                             ULyraInventoryItemInstance* NewActiveWeapon);

protected:

	/**
	 * Default number of fixed weapon slots used when UMYSTInventoryAttributeSet is not yet
	 * available at BeginPlay.  Override per-instance in Blueprints or the Experience data.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WeaponBar",
	          meta=(ClampMin=1, UIMin=1))
	int32 DefaultNumFixedSlots = 2;

	UFUNCTION()
	void OnRep_FixedSlots();

	UFUNCTION()
	void OnRep_PickupSlot();

	UFUNCTION()
	void OnRep_ActiveSlotIndex();

	/** Equips the item in the currently active slot via EquipmentManagerComponent. Authority-only. */
	virtual void EquipActiveWeapon();

	/** Unequips the currently equipped item via EquipmentManagerComponent. Authority-only. */
	virtual void UnequipActiveWeapon();

	/** Finds the ULyraEquipmentManagerComponent on the currently controlled pawn. */
	virtual ULyraEquipmentManagerComponent* FindEquipmentManager() const;

private:

	/** Ordered list of non-empty slot indices used for cycling. Pickup appended last as PickupSlotActiveIndex. */
	TArray<int32> GetAllNonEmptySlotIndices() const;

	/** Switches active slot: unequips old, updates index, equips new, broadcasts messages. */
	void ApplyActiveSlotChange(int32 NewSlotIndex);

	/** Sends FMYSTWeaponBarSlotsChangedMessage to the GameplayMessageSubsystem. */
	void BroadcastSlotsChanged();

	/** Sends FMYSTWeaponBarActiveSlotChangedMessage to the GameplayMessageSubsystem. */
	void BroadcastActiveSlotChanged();

	// ─── Replicated state ─────────────────────────────────────────────────────

	/** Permanent weapon slots; null entries indicate empty slots. Replicated. */
	UPROPERTY(ReplicatedUsing=OnRep_FixedSlots)
	TArray<TObjectPtr<ULyraInventoryItemInstance>> FixedSlots;

	/** Temporary pickup weapon slot (null = empty). Replicated. */
	UPROPERTY(ReplicatedUsing=OnRep_PickupSlot)
	TObjectPtr<ULyraInventoryItemInstance> PickupSlot;

	/**
	 * Currently active slot index.
	 *   PickupSlotActiveIndex (-2) = pickup, INDEX_NONE (-1) = none, 0+ = fixed slot.
	 * Replicated.
	 */
	UPROPERTY(ReplicatedUsing=OnRep_ActiveSlotIndex)
	int32 ActiveSlotIndex = INDEX_NONE;

	/** Server-side reference to the equipment instance currently held by this bar. Not replicated. */
	UPROPERTY()
	TObjectPtr<ULyraEquipmentInstance> EquippedItem;
};

