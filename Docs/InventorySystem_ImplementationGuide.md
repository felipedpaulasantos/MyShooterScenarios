# MYST Inventory System — Implementation Guide

> **Covers:** Fixed weapons · Pickup weapons · Potions/consumables · HUD widgets  
> **Phases implemented:** 1 (Fragments) · 2 (AttributeSet) · 3 (CapacityComponent + GA_UseItem) · 4 (WeaponBarComponent) · 5 (FunctionLibrary)

---

## Prerequisites (one-time setup per Experience)

Before any of the scenarios below work, the Experience must add the three components
to the **PlayerController** via `UGameFeatureAction_AddComponents`:

| Component | Where |
|---|---|
| `ULyraInventoryManagerComponent` | PlayerController (already present if using `B_Hero_ShooterMannequin`) |
| `UMYSTInventoryCapacityComponent` | PlayerController |
| `UMYSTWeaponBarComponent` | PlayerController |

The Experience must also **grant `UMYSTInventoryAttributeSet`** to the PlayerState's ASC
via a `ULyraAbilitySet` — this defines `MaxWeaponSlots`, `MaxPotionSlots`, `MaxAmmoCapacity`, and `MaxPickupWeapons`.

---

## Scenario 1 — Assign a weapon as a Fixed (permanent) item

### Step 1 — Set up the Item Definition asset

Create a `ULyraInventoryItemDefinition` data asset (prefix `DA_`).
Add these **Fragments** in the Fragments array:

| Fragment | Settings |
|---|---|
| **Item Category** | `ItemCategory` = `Item.Category.Weapon` |
| **Weapon Slot Type** | `SlotType` = `Fixed (Permanent)` |
| **Equippable Item** | `EquipmentDefinition` = your `ULyraEquipmentDefinition` asset |

> **Tip:** The Equippable Item fragment is what actually spawns the weapon actor and grants
> abilities via Lyra's Equipment Manager. It must point to a valid `ULyraEquipmentDefinition`.

### Step 2 — Give the weapon to the player (Blueprint)

```
[Event BeginPlay / Pickup Overlap / etc.]
    ↓
Get Player Controller (index 0)
    ↓
MYST | Give Item To Player
  · Controller  = (controller)
  · Item Def    = DA_MyRifle
  · Stack Count = 1
    ↓
(returns ULyraInventoryItemInstance*)
```

`GiveItemToPlayer` automatically:
1. Checks capacity (`MaxWeaponSlots` attribute).
2. Adds the item to the inventory.
3. Places it in the **first free fixed slot** of the Weapon Bar.

### Step 3 — Equip it (switch to it)

```
Get Weapon Bar (Controller)  →  Set Active Fixed Slot (SlotIndex = 0)
```

Or bind `CycleWeaponForward` / `CycleWeaponBackward` to scroll-wheel input.

### Step 4 — Manual slot assignment (optional)

If all slots were full when you called `GiveItemToPlayer`, the item is in inventory
but not in the bar. Assign it explicitly later:

```
MYST | Assign Weapon To Fixed Slot
  · Controller  = …
  · Weapon Item = (the instance returned earlier)
  · Slot Index  = 1
```

---

## Scenario 2 — Assign a weapon as a Pickup (disposable) item

### Step 1 — Set up the Item Definition asset

Same as Scenario 1 but change **one fragment setting**:

| Fragment | Settings |
|---|---|
| **Weapon Slot Type** | `SlotType` = `Pickup (Temporary)` |

> Also update `ItemCategory` to `Item.Category.PickupWeapon` so the capacity system
> counts it against `MaxPickupWeapons` instead of `MaxWeaponSlots`.

### Step 2 — Give the weapon to the player

```
MYST | Give Item To Player
  · Item Def    = DA_RPG_Pickup
  · Stack Count = 1
```

`GiveItemToPlayer` calls `SetPickupWeapon` on the Weapon Bar automatically,
replacing any previously held pickup.

### Step 3 — Discard when ammo runs out

Hook into whatever ammo-depleted event your weapon fires (e.g. a Gameplay Message
or a direct call from the weapon ability), then:

```
MYST | Remove Weapon From Player
  · Controller                 = …
  · Weapon Item                = (the pickup instance)
  · Also Remove From Inventory = true   ← cleans up inventory too
```

This calls `ClearPickupWeapon()` internally and auto-falls back to the first
occupied fixed slot.

> **Alternative (authority only):** Call `Get Weapon Bar → Clear Pickup Weapon` directly
> when you only want to clear the slot without touching the inventory.

---

## Scenario 3 — Add and remove potions / consumables

### Step 1 — Set up the Item Definition asset

| Fragment | Settings |
|---|---|
| **Item Category** | `ItemCategory` = `Item.Category.Potion` |
| **Usable Item** | `EffectToApply` = `GE_HealPotion` (instant GE that modifies Health) |
| | `UseMontage` = `AM_DrinkPotion` (optional) |
| | `CooldownTag` = `Ability.Cooldown.UseItem.Potion` |
| | `Stacks Consumed Per Use` = `1` |

> Potions do **not** need `InventoryFragment_WeaponSlotType` or `InventoryFragment_EquippableItem`.
> They are not weapons and the Weapon Bar ignores them entirely.

### Step 2 — Add potions to the inventory

```
MYST | Give Item To Player
  · Item Def    = DA_HealthPotion
  · Stack Count = 3
```

Capacity is enforced automatically by `MaxPotionSlots`. If the player is already at
the maximum, `GiveItemToPlayer` returns null and does nothing.

### Step 3 — Use a potion (via ability)

Grant `GA_UseItem` (or a Blueprint subclass) to the pawn via the Experience `ULyraAbilitySet`.
When activated the ability automatically:
1. Finds the first item in inventory that has `UInventoryFragment_UsableItem`.
2. Applies `EffectToApply` to the avatar's ASC.
3. Removes `StacksConsumedPerUse` instances from the inventory.
4. Starts the per-fragment cooldown.

> To select a **specific** potion type (health vs energy) instead of the first found,
> subclass `GA_UseItem` in C++ or Blueprint and override `GetBestUsableItem`.

### Step 4 — Remove potions manually (death / sell / script)

To remove a single instance:
```
Get Inventory Manager (Controller)  →  Remove Item Instance (ItemInstance)
```

To remove **all** potions at once:
```
MYST | Find Items By Category
  · Category = Item.Category.Potion
    ↓
ForEach Loop  →  Get Inventory Manager → Remove Item Instance
```

---

## Scenario 4 — HUD Widget (overview)

The recommended approach is a **UMG Widget** that listens to
`UGameplayMessageSubsystem` messages rather than polling on Tick.

### Step 1 — Subscribe to messages in `Event Construct`

```
[Event Construct]
    ↓
Get Gameplay Message Subsystem
    ↓
Register Listener
  · Channel   = MYST.WeaponBar.Message.SlotsChanged
  · On Message = (custom event) OnWeaponBarSlotsChanged
      Payload type: FMYSTWeaponBarSlotsChangedMessage

Register Listener
  · Channel   = MYST.WeaponBar.Message.ActiveSlotChanged
  · On Message = (custom event) OnActiveSlotChanged
      Payload type: FMYSTWeaponBarActiveSlotChangedMessage
```

Store both listener handles and **unregister them in `Event Destruct`**.

### Step 2 — Draw the weapon bar (fixed slots + pickup slot)

In `OnWeaponBarSlotsChanged(Payload)`:

```
ForEachLoop on Payload.FixedSlots  (index 0 … N)
    ↓
    Slot[i] is valid?
        YES → show weapon icon  (MYST | Get Item Display Name, or QuickBarIcon fragment brush)
        NO  → show empty-slot graphic

Payload.PickupSlot is valid?
    YES → show pickup weapon HUD element
    NO  → hide pickup weapon HUD element
```

In `OnActiveSlotChanged(Payload)`:

```
Active = Payload.ActiveSlotIndex

Active == MYST | Get Pickup Slot Active Index  (-2)?  → highlight pickup slot widget
Active >= 0?                                          → highlight fixed slot widget [Active]
else                                                  → clear all highlights
```

### Step 3 — Draw the potion counter

Subscribe to `Lyra.Inventory.Message.StackChanged`
(broadcast by `ULyraInventoryManagerComponent`) to react to any inventory change:

```
[On Inventory Stack Changed]
    ↓
MYST | Find Items By Category
  · Controller = Get Owning Player
  · Category   = Item.Category.Potion
    ↓
Array Length  →  Set Text "x{N}" on potion counter widget

MYST | Get Inventory Capacity Component
    ↓
Get Remaining Capacity For Category (Item.Category.Potion)
    →  Show "N / Max" or colour counter red when remaining == 0
```

### Step 4 — Initial population on first frame

On `Event Construct` (after subscriptions), do a one-time refresh so the HUD
shows correct state before the first message fires:

```
[Event Construct — after subscriptions]
    ↓
MYST | Get Weapon Bar (Controller)
    ↓ (is valid branch)
Get Fixed Slots   →  run the same slot-drawing loop as Step 2
Get Pickup Weapon →  show/hide pickup element
Get Active Slot Index  →  apply highlight
```

---

## Fragment Cheat Sheet

| Use case | Required Fragments |
|---|---|
| Fixed weapon | `ItemCategory` (`Item.Category.Weapon`) + `WeaponSlotType` (`Fixed`) + `EquippableItem` |
| Pickup weapon | `ItemCategory` (`Item.Category.PickupWeapon`) + `WeaponSlotType` (`Pickup`) + `EquippableItem` |
| Potion / consumable | `ItemCategory` (`Item.Category.Potion`) + `UsableItem` |
| Ammo | `ItemCategory` (`Item.Category.Ammo`) |
| Utility (grenades, gadgets) | `ItemCategory` (`Item.Category.Utility`) — no capacity limit |

---

## Key Blueprint Nodes (MYST category)

| Node | Group | Notes |
|---|---|---|
| `Give Item To Player` | Operations | Add to inventory **and** auto-assign to weapon bar |
| `Remove Weapon From Player` | Operations | Remove from bar + optionally from inventory |
| `Assign Weapon To Fixed Slot` | Operations | Manually place an inventory item into a specific bar slot |
| `Can Give Item To Player` | Queries | Dry-run capacity check — no mutation |
| `Find Items By Category` | Queries | Returns all inventory items of a given `Item.Category.*` tag |
| `Find Weapon Bar Slot For Item` | Queries | Outputs `bIsPickupSlot` + `FixedSlotIndex` |
| `Get Weapon Bar (Controller/Pawn)` | Accessors | Get the `UMYSTWeaponBarComponent` |
| `Get Inventory Manager (Controller/Pawn)` | Accessors | Get `ULyraInventoryManagerComponent` |
| `Get Inventory Capacity Component` | Accessors | Get `UMYSTInventoryCapacityComponent` |
| `Get Inventory Attribute Set` | Accessors | Get `UMYSTInventoryAttributeSet` (read capacity values) |
| `Is Fixed Weapon / Is Pickup Weapon / Is Usable Item` | Item queries | Fragment presence checks |
| `Get Item Category / Get Weapon Slot Type` | Item queries | Fragment data reads |
| `Get Item Display Name` | Item queries | Reads `DisplayName` from the item definition CDO |

---

## Upgrading Capacity at Runtime

Apply a Gameplay Effect to the PlayerState's ASC that modifies the relevant attribute:

| What to upgrade | Attribute to modify |
|---|---|
| More fixed weapon slots | `UMYSTInventoryAttributeSet.MaxWeaponSlots` |
| More potion slots | `UMYSTInventoryAttributeSet.MaxPotionSlots` |
| More ammo capacity | `UMYSTInventoryAttributeSet.MaxAmmoCapacity` |

After applying the GE, if you also want the Weapon Bar to resize:
```
MYST | Get Weapon Bar (Controller)  →  Resize Fixed Slots (NewCount)
```

