// Copyright MyShooterScenarios. All Rights Reserved.

#include "Inventory/MYSTInventoryCapacityComponent.h"
#include "AbilitySystem/MYSTInventoryAttributeSet.h"
#include "Inventory/InventoryFragment_ItemCategory.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Inventory/LyraInventoryManagerComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MYSTInventoryCapacityComponent)

// ─────────────────────────────────────────────────────────────────────────────
// UMYSTInventoryCapacityComponent
// ─────────────────────────────────────────────────────────────────────────────

UMYSTInventoryCapacityComponent::UMYSTInventoryCapacityComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UMYSTInventoryCapacityComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ULyraInventoryManagerComponent* InventoryManager = GetInventoryManager())
	{
		InventoryManager->OnCanAddItem.AddDynamic(this, &UMYSTInventoryCapacityComponent::HandleCanAddItem);
	}
}

void UMYSTInventoryCapacityComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unbind to avoid dangling delegate references
	if (ULyraInventoryManagerComponent* InventoryManager = GetInventoryManager())
	{
		InventoryManager->OnCanAddItem.RemoveDynamic(this, &UMYSTInventoryCapacityComponent::HandleCanAddItem);
	}

	Super::EndPlay(EndPlayReason);
}

// ─────────────────────────────────────────────────────────────────────────────
// Capacity veto handler
// ─────────────────────────────────────────────────────────────────────────────

void UMYSTInventoryCapacityComponent::HandleCanAddItem(
	TSubclassOf<ULyraInventoryItemDefinition> ItemDef,
	int32 StackCount,
	bool& bCanAdd)
{
	// Respect vetoes set by earlier bindings in the same broadcast
	if (!bCanAdd || !IsValid(ItemDef))
	{
		return;
	}

	// Read the category fragment from the item's class-default-object
	const ULyraInventoryItemDefinition* ItemCDO = GetDefault<ULyraInventoryItemDefinition>(ItemDef);
	if (!ItemCDO)
	{
		return;
	}

	const UInventoryFragment_ItemCategory* CategoryFragment =
		Cast<UInventoryFragment_ItemCategory>(
			ItemCDO->FindFragmentByClass(UInventoryFragment_ItemCategory::StaticClass()));

	// Items without a category fragment bypass capacity checks
	if (!CategoryFragment || !CategoryFragment->ItemCategory.IsValid())
	{
		return;
	}

	const FGameplayTag Category = CategoryFragment->ItemCategory;
	const float MaxCapacity = GetMaxCapacityForCategory(Category);

	// MaxCapacity == -1 means this category is unlimited
	if (MaxCapacity < 0.0f)
	{
		return;
	}

	const int32 CurrentCount = GetCurrentCountForCategory(Category);
	if (static_cast<float>(CurrentCount + StackCount) > MaxCapacity)
	{
		bCanAdd = false;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Blueprint-callable queries
// ─────────────────────────────────────────────────────────────────────────────

int32 UMYSTInventoryCapacityComponent::GetCurrentCountForCategory(FGameplayTag Category) const
{
	const ULyraInventoryManagerComponent* InventoryManager = GetInventoryManager();
	if (!InventoryManager)
	{
		return 0;
	}

	int32 Count = 0;
	for (ULyraInventoryItemInstance* Instance : InventoryManager->GetAllItems())
	{
		if (!IsValid(Instance))
		{
			continue;
		}

		const UInventoryFragment_ItemCategory* Frag =
			Instance->FindFragmentByClass<UInventoryFragment_ItemCategory>();

		if (Frag && Frag->ItemCategory == Category)
		{
			++Count;
		}
	}
	return Count;
}

float UMYSTInventoryCapacityComponent::GetMaxCapacityForCategory(FGameplayTag Category) const
{
	if (const UMYSTInventoryAttributeSet* AttrSet = GetAttributeSet())
	{
		return AttrSet->GetCapacityForCategory(Category);
	}

	// No attribute set present (e.g. pre-Experience, PIE startup) → unlimited
	return -1.0f;
}

float UMYSTInventoryCapacityComponent::GetRemainingCapacityForCategory(FGameplayTag Category) const
{
	const float Max = GetMaxCapacityForCategory(Category);
	if (Max < 0.0f)
	{
		return -1.0f; // Unlimited
	}
	return Max - static_cast<float>(GetCurrentCountForCategory(Category));
}

bool UMYSTInventoryCapacityComponent::CanAddItemOfCategory(FGameplayTag Category) const
{
	const float Remaining = GetRemainingCapacityForCategory(Category);
	return Remaining < 0.0f || Remaining >= 1.0f;
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

ULyraInventoryManagerComponent* UMYSTInventoryCapacityComponent::GetInventoryManager() const
{
	AController* MyController = Cast<AController>(GetOwner());
	return MyController
		? MyController->GetComponentByClass<ULyraInventoryManagerComponent>()
		: nullptr;
}

const UMYSTInventoryAttributeSet* UMYSTInventoryCapacityComponent::GetAttributeSet() const
{
	const AController* MyController = Cast<AController>(GetOwner());
	if (!MyController)
	{
		return nullptr;
	}

	const APlayerState* PS = MyController->GetPlayerState<APlayerState>();
	if (!PS)
	{
		return nullptr;
	}

	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PS);
	if (!ASC)
	{
		return nullptr;
	}

	return ASC->GetSet<UMYSTInventoryAttributeSet>();
}

