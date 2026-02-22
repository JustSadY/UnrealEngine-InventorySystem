#pragma once

#include "CoreMinimal.h"
#include "InventorySlots.h"
#include "Items/ItemBase.h"
#include "InventorySlotsGroup.generated.h"

/**
 * Multiplayer compatible inventory group manager.
 * Uses TArray for replication support while maintaining TypeID-based logic.
 */
USTRUCT(BlueprintType)
struct FInventorySlotsGroup
{
	GENERATED_BODY()

private:
	/** Using TArray because TMap is not supported for Replication */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	TArray<FInventorySlots> InventoryGroups;

	/** Cache for quick lookups - maps TypeID to array index */
	mutable TMap<int32, int32> TypeIDToIndexCache;
	
	/** Flag to track if cache needs rebuilding */
	mutable bool bCacheNeedsRebuild = true;

public:
	FInventorySlotsGroup() = default;

	/**
	 * Adds a new group to the collection.
	 * @param NewSlots The inventory slots structure to add.
	 */
	void AddInventoryGroup(const FInventorySlots& NewSlots)
	{
		InventoryGroups.Add(NewSlots);
		bCacheNeedsRebuild = true;
	}

	/**
	 * Rebuilds the TypeID cache for faster lookups
	 */
	void RebuildCache() const
	{
		TypeIDToIndexCache.Empty();
		for (int32 i = 0; i < InventoryGroups.Num(); ++i)
		{
			TArray<int32> Keys;
			InventoryGroups[i].GetTypeIDMap().GetKeys(Keys);
			for (int32 TypeID : Keys)
			{
				TypeIDToIndexCache.Add(TypeID, i);
			}
		}
		bCacheNeedsRebuild = false;
	}

	/**
	 * Finds a group by its primary TypeID (const version).
	 * @param TypeID The unique identifier for the group.
	 * @return Pointer to the group, or nullptr if not found.
	 */
	const FInventorySlots* GetGroupByID(int32 TypeID) const
	{
		if (bCacheNeedsRebuild)
		{
			RebuildCache();
		}

		const int32* IndexPtr = TypeIDToIndexCache.Find(TypeID);
		if (IndexPtr && InventoryGroups.IsValidIndex(*IndexPtr))
		{
			return &InventoryGroups[*IndexPtr];
		}

		return nullptr;
	}

	/**
	 * Finds a group by its primary TypeID (non-const version).
	 * @param TypeID The unique identifier for the group.
	 * @return Pointer to the group, or nullptr if not found.
	 */
	FInventorySlots* GetGroupByID(int32 TypeID)
	{
		if (bCacheNeedsRebuild)
		{
			RebuildCache();
		}

		const int32* IndexPtr = TypeIDToIndexCache.Find(TypeID);
		if (IndexPtr && InventoryGroups.IsValidIndex(*IndexPtr))
		{
			return &InventoryGroups[*IndexPtr];
		}

		return nullptr;
	}

	/**
	 * Optimized AddItem logic for replicated arrays with structured response.
	 * @param ItemBase Item to add.
	 * @param TargetTypeID Specific group to target.
	 * @return FInventoryOperationResult indicating the status of the addition.
	 */
	FInventoryOperationResult AddItem(UItemBase* ItemBase, int32 TargetTypeID = -1)
	{
		if (!IsValid(ItemBase))
		{
			return FInventoryOperationResult::Fail(TEXT("Cannot add an invalid or null item."));
		}

		if (TargetTypeID != -1)
		{
			FInventorySlots* TargetGroup = GetGroupByID(TargetTypeID);
			if (!TargetGroup)
			{
				return FInventoryOperationResult::Fail(TEXT("Target inventory group not found."));
			}

			if (!TargetGroup->IsTypeSupported(ItemBase))
			{
				return FInventoryOperationResult::Fail(TEXT("The item type is not compatible with the target group."));
			}

			return TargetGroup->AddItem(ItemBase);
		}

		for (FInventorySlots& Group : InventoryGroups)
		{
			if (Group.IsTypeSupported(ItemBase))
			{
				FInventoryOperationResult Result = Group.AddItem(ItemBase);
				if (Result.bSuccess)
				{
					return Result;
				}
			}
		}

		return FInventoryOperationResult::Fail(TEXT("No suitable group found or all compatible groups are full."));
	}

	/**
	 * Transfers an item between groups with detailed result reporting.
	 */
	FInventoryOperationResult TransferItem(int32 FromTypeID, int32 FromIndex, int32 ToTypeID, int32 ToIndex)
	{
		FInventorySlots* SourceGroup = GetGroupByID(FromTypeID);
		FInventorySlots* DestGroup = GetGroupByID(ToTypeID);

		if (!SourceGroup || !DestGroup)
		{
			return FInventoryOperationResult::Fail(
				TEXT("One of the inventory groups involved in the transfer was not found."));
		}

		const FInventorySlot* SourceSlot = SourceGroup->GetSlotAtIndex(FromIndex);
		if (!SourceSlot || SourceSlot->IsEmpty())
		{
			return FInventoryOperationResult::Fail(TEXT("Source slot is empty or invalid."));
		}

		UItemBase* ItemToMove = SourceSlot->GetItem();
		if (!DestGroup->IsTypeSupported(ItemToMove))
		{
			return FInventoryOperationResult::Fail(
				TEXT("The item cannot be moved because the destination group does not support its type."));
		}

		UItemBase* RemovedItem = SourceGroup->RemoveItem(FromIndex);
		if (!RemovedItem)
		{
			return FInventoryOperationResult::Fail(TEXT("Failed to remove item from source slot."));
		}

		FInventoryOperationResult AddResult = DestGroup->AddItemToSlot(RemovedItem, ToIndex);

		if (!AddResult.bSuccess)
		{
			SourceGroup->AddItemToSlot(RemovedItem, FromIndex);
			return FInventoryOperationResult::Fail(TEXT("Transfer failed: ") + AddResult.Message);
		}

		return FInventoryOperationResult::Ok();
	}

	/**
	* Searches for items across all groups based on a name query.
	* @param SearchName The partial or full name of the item to find.
	* @return Array of pointers to slots containing matching items.
	*/
	TArray<const FInventorySlot*> FindItemsByName(const FString& SearchName) const
	{
		TArray<const FInventorySlot*> FoundSlots;

		for (const FInventorySlots& Group : InventoryGroups)
		{
			for (const FInventorySlot& Slot : Group.GetSlots())
			{
				if (!Slot.IsEmpty())
				{
					FString ItemDisplayName = Slot.GetItem()->GetItemDefinition().GetItemName().ToString();
					if (ItemDisplayName.Contains(SearchName))
					{
						FoundSlots.Add(&Slot);
					}
				}
			}
		}

		return FoundSlots;
	}

	const FInventorySlots* GetItemsByTypeID(int32 CategoryID) const
	{
		return GetGroupByID(CategoryID);
	}

	FInventorySlots* GetItemsByTypeID(int32 CategoryID)
	{
		return GetGroupByID(CategoryID);
	}

	/**
	 * Calculates the total quantity of a specific item across all managed groups.
	 * Useful for crafting or quest requirements.
	 * @param ItemID The unique FName identifier of the item.
	 * @return Total count of the item in the entire inventory system.
	 */
	int32 GetGlobalTotalItemCount(const FString& ItemID) const
	{
		int32 GrandTotal = 0;

		for (const FInventorySlots& Group : InventoryGroups)
		{
			GrandTotal += Group.GetTotalItemCount(ItemID);
		}

		return GrandTotal;
	}

	/**
	 * Consolidates stacks and compacts items to the front in all groups.
	 */
	void OrganizeAll()
	{
		for (FInventorySlots& Group : InventoryGroups)
		{
			Group.ConsolidateAndSort();
		}
	}

	FORCEINLINE const TArray<FInventorySlots>& GetInventoryGroups() const
	{
		return InventoryGroups;
	}

	FORCEINLINE TArray<FInventorySlots>& GetInventoryGroups()
	{
		bCacheNeedsRebuild = true;
		return InventoryGroups;
	}

	const FInventorySlots* GetGroupByIndex(int32 Index) const
	{
		return InventoryGroups.IsValidIndex(Index) ? &InventoryGroups[Index] : nullptr;
	}

	FInventorySlots* GetGroupByIndex(int32 Index)
	{
		return InventoryGroups.IsValidIndex(Index) ? &InventoryGroups[Index] : nullptr;
	}

	/** Call this after directly modifying InventoryGroups to force a cache rebuild on next lookup. */
	void InvalidateCache()
	{
		bCacheNeedsRebuild = true;
	}

	/**
	 * Gets the primary TypeID for a group at the given array index.
	 * Returns the first key from the group's TypeIDMap, or -1 if not found.
	 */
	int32 GetTypeIDForGroupIndex(int32 GroupArrayIndex) const
	{
		if (!InventoryGroups.IsValidIndex(GroupArrayIndex))
		{
			return -1;
		}

		const TMap<int32, FString>& TypeIDMap = InventoryGroups[GroupArrayIndex].GetTypeIDMap();
		auto It = TypeIDMap.CreateConstIterator();
		return It ? It.Key() : -1;
	}

	/**
	 * Finds the TypeID and slot index for a given item across all groups.
	 * Returns true if found, populating OutTypeID and OutSlotIndex.
	 */
	bool FindItemLocation(const UItemBase* Item, int32& OutTypeID, int32& OutSlotIndex) const
	{
		if (!IsValid(Item))
		{
			return false;
		}

		for (int32 GroupIdx = 0; GroupIdx < InventoryGroups.Num(); ++GroupIdx)
		{
			const TArray<FInventorySlot>& Slots = InventoryGroups[GroupIdx].GetSlots();
			for (int32 SIdx = 0; SIdx < Slots.Num(); ++SIdx)
			{
				if (Slots[SIdx].GetItem() == Item)
				{
					OutTypeID = GetTypeIDForGroupIndex(GroupIdx);
					OutSlotIndex = SIdx;
					return true;
				}
			}
		}
		return false;
	}
};