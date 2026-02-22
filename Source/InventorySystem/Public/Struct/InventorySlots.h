#pragma once

#include "CoreMinimal.h"
#include "InventoryOperationResult.h"
#include "InventorySlot.h"
#include "Items/ItemBase.h"
#include "InventorySlots.generated.h"

/**
 * Manages a collection of inventory slots with support for type validation, stacking, and sorting.
 */
USTRUCT(BlueprintType)
struct FInventorySlots
{
	GENERATED_BODY()

private:
	/** Maximum number of slots in this inventory */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 MaxSlotSize = 64;

	/** Map containing supported type IDs for this inventory group */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TMap<int32, FString> TypeIDMap;

	/** Internal collection of inventory slots */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TArray<FInventorySlot> Slots;

public:
	FInventorySlots() = default;

	/**
	 * Initializes the inventory slots array and type restrictions.
	 * @param Size The number of slots to create.
	 * @param NewTypeIDMap Map containing supported type IDs.
	 */
	void InitializeInventory(int32 Size, const TMap<int32, FString>& NewTypeIDMap)
	{
		MaxSlotSize = Size;
		TypeIDMap = NewTypeIDMap;
		Slots.Empty(MaxSlotSize);
		Slots.SetNum(MaxSlotSize);
	}

	/**
	 * Checks if the item category is allowed in this slot group.
	 * @param Item The item to validate.
	 * @return True if the item type is found in the TypeIDMap.
	 */
	bool IsTypeSupported(const UItemBase* Item) const
	{
		if (!IsValid(Item))
		{
			return false;
		}

		const TArray<int32>& ItemTypeIDs = Item->GetItemDefinition().GetInventorySlotTypeIDs();

		for (int32 ID : ItemTypeIDs)
		{
			if (TypeIDMap.Contains(ID))
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * Logic for adding an item, handling stack overflows and empty slot searches.
	 * @param NewItem The item object to add.
	 * @return An FInventoryOperationResult indicating the outcome of the addition attempt.
	 */
	FInventoryOperationResult AddItem(UItemBase* NewItem)
	{
		if (!IsValid(NewItem))
		{
			return FInventoryOperationResult::Fail(TEXT("The item you are trying to add is invalid"));
		}

		if (!IsTypeSupported(NewItem))
		{
			return FInventoryOperationResult::Fail(TEXT("The item type is not supported by this inventory"));
		}

		int32 RemainingToStack = NewItem->GetCurrentStackSize();

		if (NewItem->IsStackable())
		{
			for (FInventorySlot& Slot : Slots)
			{
				if (!Slot.IsEmpty() &&
					Slot.GetItem()->GetItemDefinition().GetItemID() == NewItem->GetItemDefinition().GetItemID())
				{
					if (!Slot.IsFull())
					{
						RemainingToStack = Slot.AddToStack(RemainingToStack);

						if (RemainingToStack <= 0)
						{
							return FInventoryOperationResult::Ok();
						}
					}
				}
			}
		}

		if (RemainingToStack > 0)
		{
			for (FInventorySlot& Slot : Slots)
			{
				if (Slot.IsEmpty())
				{
					Slot.SetItem(NewItem, RemainingToStack);
					return FInventoryOperationResult::Ok();
				}
			}
		}

		NewItem->SetCurrentStackSize(RemainingToStack);

		return FInventoryOperationResult::Fail(
			TEXT("Inventory is full. Remaining items were left in the source object"));
	}

	/**
	 * Attempts to add an item to a specific slot index.
	 * @param NewItem The item object to add.
	 * @param TargetIndex The preferred slot index.
	 * @return An FInventoryOperationResult indicating if the item was placed or re-routed.
	 */
	FInventoryOperationResult AddItemToSlot(UItemBase* NewItem, int32 TargetIndex)
	{
		if (!IsValid(NewItem))
		{
			return FInventoryOperationResult::Fail(TEXT("Provided item is invalid"));
		}

		if (!Slots.IsValidIndex(TargetIndex))
		{
			return FInventoryOperationResult::Fail(TEXT("Target slot index is out of bounds"));
		}

		if (!IsTypeSupported(NewItem))
		{
			return FInventoryOperationResult::Fail(TEXT("Item type is not supported by this inventory"));
		}

		FInventorySlot& TargetSlot = Slots[TargetIndex];

		if (TargetSlot.IsEmpty())
		{
			TargetSlot.SetItem(NewItem, NewItem->GetCurrentStackSize());
			return FInventoryOperationResult::Ok();
		}

		if (NewItem->IsStackable() &&
			TargetSlot.GetItem()->GetItemDefinition().GetItemID() == NewItem->GetItemDefinition().GetItemID())
		{
			if (!TargetSlot.IsFull())
			{
				int32 Overflow = TargetSlot.AddToStack(NewItem->GetCurrentStackSize());

				if (Overflow <= 0)
				{
					return FInventoryOperationResult::Ok();
				}

				NewItem->SetCurrentStackSize(Overflow);
				return AddItem(NewItem);
			}
		}

		return AddItem(NewItem);
	}

	/**
	 * Clears a slot and returns the item reference.
	 * @param Index Target slot index.
	 * @return Pointer to the item previously in the slot.
	 */
	UItemBase* RemoveItem(int32 Index)
	{
		if (Slots.IsValidIndex(Index) && !Slots[Index].IsEmpty())
		{
			UItemBase* RemovedItem = Slots[Index].GetItem();
			Slots[Index].ClearSlot();
			return RemovedItem;
		}

		return nullptr;
	}

	/**
	 * Resets all slots to their default empty state.
	 */
	void ClearAllSlots()
	{
		for (FInventorySlot& Slot : Slots)
		{
			Slot.ClearSlot();
		}
	}

	/**
	 * Exchanges data between two slots.
	 * @param IndexA First slot index.
	 * @param IndexB Second slot index.
	 * @return FInventoryOperationResult indicating the outcome.
	 */
	FInventoryOperationResult SwapSlots(int32 IndexA, int32 IndexB)
	{
		if (!Slots.IsValidIndex(IndexA) || !Slots.IsValidIndex(IndexB))
		{
			return FInventoryOperationResult::Fail(
				FString::Printf(TEXT("Invalid slot index: A=%d, B=%d"), IndexA, IndexB));
		}

		Slots.Swap(IndexA, IndexB);
		return FInventoryOperationResult::Ok();
	}

	/**
	 * Splits a stack into two and moves the specified amount to a new slot.
	 * Uses UItemBase::SplitStack to create a proper new item instance.
	 * @param SourceIndex The index of the slot to split from.
	 * @param TargetIndex The index where the split portion will be placed.
	 * @param Amount The quantity to move.
	 * @return FInventoryOperationResult indicating the outcome.
	 */
	FInventoryOperationResult SplitStack(int32 SourceIndex, int32 TargetIndex, int32 Amount)
	{
		if (!Slots.IsValidIndex(SourceIndex) || !Slots.IsValidIndex(TargetIndex))
		{
			return FInventoryOperationResult::Fail(TEXT("Invalid source or target slot index"));
		}

		if (Slots[SourceIndex].IsEmpty())
		{
			return FInventoryOperationResult::Fail(TEXT("Source slot is empty"));
		}

		if (!Slots[TargetIndex].IsEmpty())
		{
			return FInventoryOperationResult::Fail(TEXT("Target slot is not empty"));
		}

		if (Amount <= 0)
		{
			return FInventoryOperationResult::Fail(TEXT("Split amount must be greater than zero"));
		}

		if (Slots[SourceIndex].GetCurrentStackSize() <= Amount)
		{
			return FInventoryOperationResult::Fail(TEXT("Split amount must be less than current stack size"));
		}

		UItemBase* OriginalItem = Slots[SourceIndex].GetItem();
		if (!OriginalItem)
		{
			return FInventoryOperationResult::Fail(TEXT("Source slot item is invalid"));
		}

		UItemBase* NewItem = OriginalItem->SplitStack(Amount);
		if (!NewItem)
		{
			return FInventoryOperationResult::Fail(TEXT("Failed to create split item instance"));
		}

		Slots[SourceIndex].SetItem(OriginalItem, OriginalItem->GetCurrentStackSize());
		Slots[TargetIndex].SetItem(NewItem, Amount);

		return FInventoryOperationResult::Ok();
	}

	/**
	 * Consolidates stackable items to maximize free space and compacts to front.
	 */
	void ConsolidateAndSort()
	{
		ConsolidateStacks();

		int32 WriteIndex = 0;
		for (int32 ReadIndex = 0; ReadIndex < Slots.Num(); ++ReadIndex)
		{
			if (!Slots[ReadIndex].IsEmpty())
			{
				if (WriteIndex != ReadIndex)
				{
					Slots.Swap(WriteIndex, ReadIndex);
				}
				WriteIndex++;
			}
		}
	}

	/**
	 * Retrieves a slot at a specific index.
	 * @param Index The index of the slot to retrieve.
	 * @return A pointer to the slot, or nullptr if index is invalid.
	 */
	const FInventorySlot* GetSlotAtIndex(int32 Index) const
	{
		if (Slots.IsValidIndex(Index))
		{
			return &Slots[Index];
		}

		return nullptr;
	}

	/**
	 * Finds the first slot containing an item with a specific ID.
	 * @param ItemID The unique identifier of the item.
	 * @return A pointer to the found slot, or nullptr if not found.
	 */
	const FInventorySlot* FindSlotByItemID(const FString& ItemID) const
	{
		for (const FInventorySlot& Slot : Slots)
		{
			if (!Slot.IsEmpty() && Slot.GetItem()->GetItemDefinition().GetItemID() == ItemID)
			{
				return &Slot;
			}
		}

		return nullptr;
	}

	/**
	 * Calculates the total quantity of a specific item across all slots.
	 * @param ItemID The unique identifier of the item.
	 * @return Total count of the item.
	 */
	int32 GetTotalItemCount(const FString& ItemID) const
	{
		int32 Total = 0;

		for (const FInventorySlot& Slot : Slots)
		{
			if (!Slot.IsEmpty() && Slot.GetItem()->GetItemDefinition().GetItemID() == ItemID)
			{
				Total += Slot.GetCurrentStackSize();
			}
		}

		return Total;
	}

	/**
	 * Gets the number of slots containing items.
	 * @return Integer count of non-empty slots.
	 */
	int32 GetOccupiedSlotCount() const
	{
		int32 Count = 0;

		for (const FInventorySlot& Slot : Slots)
		{
			if (!Slot.IsEmpty())
			{
				Count++;
			}
		}

		return Count;
	}

	/**
	 * Decreases the stack size of an item at a specific slot.
	 * @param SlotIndex The index of the slot to modify.
	 * @param Amount Quantity to remove.
	 * @return FInventoryOperationResult indicating the outcome.
	 */
	FInventoryOperationResult RemoveStackAmountFromSlot(int32 SlotIndex, int32 Amount)
	{
		if (!Slots.IsValidIndex(SlotIndex))
		{
			return FInventoryOperationResult::Fail(FString::Printf(TEXT("Invalid slot index %d"), SlotIndex));
		}

		if (Slots[SlotIndex].IsEmpty())
		{
			return FInventoryOperationResult::Fail(TEXT("Slot is empty"));
		}

		if (Amount <= 0)
		{
			return FInventoryOperationResult::Fail(TEXT("Amount must be greater than zero"));
		}

		int32 Removed = Slots[SlotIndex].RemoveFromStack(Amount);
		if (Removed > 0)
		{
			return FInventoryOperationResult::Ok();
		}

		return FInventoryOperationResult::Fail(TEXT("Failed to remove stack amount"));
	}

	/**
	 * Completely deletes an item from a specific slot regardless of stack size.
	 * @param SlotIndex The index of the slot to clear.
	 * @return FInventoryOperationResult indicating the outcome.
	 */
	FInventoryOperationResult DestroyItemAtSlot(int32 SlotIndex)
	{
		if (!Slots.IsValidIndex(SlotIndex))
		{
			return FInventoryOperationResult::Fail(FString::Printf(TEXT("Invalid slot index %d"), SlotIndex));
		}

		if (Slots[SlotIndex].IsEmpty())
		{
			return FInventoryOperationResult::Fail(TEXT("Slot is already empty"));
		}

		Slots[SlotIndex].ClearSlot();
		return FInventoryOperationResult::Ok();
	}

	/**
	 * Attempts to add a specific amount to an existing stack or a new slot.
	 * @param NewItem The item type to add.
	 * @param Amount How many units to add.
	 * @return Remaining amount that couldn't be added (0 if successful).
	 */
	int32 AddStackAmount(UItemBase* NewItem, int32 Amount)
	{
		if (!IsValid(NewItem) || !IsTypeSupported(NewItem) || Amount <= 0)
		{
			return Amount;
		}

		int32 Remaining = Amount;

		if (NewItem->IsStackable())
		{
			for (FInventorySlot& Slot : Slots)
			{
				if (!Slot.IsEmpty() && Slot.GetItem()->GetItemDefinition().GetItemID() == NewItem->GetItemDefinition().
					GetItemID())
				{
					Remaining = Slot.AddToStack(Remaining);
					if (Remaining <= 0) return 0;
				}
			}
		}

		if (Remaining > 0)
		{
			for (FInventorySlot& Slot : Slots)
			{
				if (Slot.IsEmpty())
				{
					Slot.SetItem(NewItem, Remaining);
					return 0;
				}
			}
		}

		return Remaining;
	}

	FORCEINLINE bool IsFull() const
	{
		return GetFreeSlotCount() == 0;
	}

	FORCEINLINE int32 GetFreeSlotCount() const
	{
		return MaxSlotSize - GetOccupiedSlotCount();
	}

	FORCEINLINE int32 GetMaxSlotSize() const
	{
		return MaxSlotSize;
	}

	FORCEINLINE const TMap<int32, FString>& GetTypeIDMap() const
	{
		return TypeIDMap;
	}

	FORCEINLINE const TArray<FInventorySlot>& GetSlots() const
	{
		return Slots;
	}

	FORCEINLINE TArray<FInventorySlot>& GetSlotsMutable()
	{
		return Slots;
	}

private:
	/**
	 * Merges partial stacks of the same item type together.
	 */
	void ConsolidateStacks()
	{
		for (int32 i = 0; i < Slots.Num(); ++i)
		{
			if (Slots[i].IsEmpty() || !Slots[i].GetItem()->IsStackable() || Slots[i].IsFull())
			{
				continue;
			}

			for (int32 j = i + 1; j < Slots.Num(); ++j)
			{
				if (Slots[j].IsEmpty() ||
					Slots[j].GetItem()->GetItemDefinition().GetItemID() != Slots[i].GetItem()->GetItemDefinition().
					GetItemID())
				{
					continue;
				}

				int32 RoomInI = Slots[i].GetAvailableSpace();
				int32 AmountInJ = Slots[j].GetCurrentStackSize();
				int32 TransferAmount = FMath::Min(RoomInI, AmountInJ);

				Slots[i].AddToStack(TransferAmount);
				Slots[j].RemoveFromStack(TransferAmount);

				if (Slots[i].IsFull())
				{
					break;
				}
			}
		}
	}
};
