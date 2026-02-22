#pragma once

#include "CoreMinimal.h"
#include "Items/ItemBase.h"
#include "InventorySlot.generated.h"

/**
 * A single slot within an inventory.
 * Keeps Item, CurrentStackSize, and MaxStackSize in sync to ensure data consistency.
 */
USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UItemBase> Item;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 CurrentStackSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 MaxStackSize;

public:
	FInventorySlot()
		: Item(nullptr), CurrentStackSize(0), MaxStackSize(1)
	{
	}

	FORCEINLINE UItemBase* GetItem() const { return Item; }
	FORCEINLINE int32 GetCurrentStackSize() const { return CurrentStackSize; }
	FORCEINLINE int32 GetMaxStackSize() const { return MaxStackSize; }
	FORCEINLINE bool IsEmpty() const { return Item == nullptr || CurrentStackSize <= 0; }
	FORCEINLINE bool IsFull() const { return CurrentStackSize >= MaxStackSize; }
	FORCEINLINE int32 GetAvailableSpace() const { return MaxStackSize - CurrentStackSize; }

	bool CanStackItem(const UItemBase* InItem) const
	{
		if (IsEmpty() || !IsValid(InItem) || IsFull())
		{
			return false;
		}

		return Item->GetItemDefinition().GetItemID() == InItem->GetItemDefinition().GetItemID();
	}

	bool CanAcceptItem(const UItemBase* InItem) const
	{
		if (!IsValid(InItem))
		{
			return false;
		}

		if (IsEmpty())
		{
			return true;
		}

		return CanStackItem(InItem);
	}

	/** Sets the item and syncs MaxStackSize from the item's data. Clears slot if item is invalid. */
	void SetItem(UItemBase* InItem, int32 InQuantity = 1)
	{
		if (!IsValid(InItem))
		{
			ClearSlot();
			return;
		}

		Item = InItem;
		MaxStackSize = InItem->GetMaxStackSize();
		CurrentStackSize = FMath::Clamp(InQuantity, 0, MaxStackSize);
	}

	void ClearSlot()
	{
		Item = nullptr;
		CurrentStackSize = 0;
		MaxStackSize = 1;
	}

	/** Adds items to the stack. Returns the overflow amount that did not fit. */
	int32 AddToStack(int32 AmountToAdd)
	{
		if (AmountToAdd <= 0 || IsEmpty())
		{
			return AmountToAdd;
		}

		const int32 ActualAdd = FMath::Min(AmountToAdd, GetAvailableSpace());
		CurrentStackSize += ActualAdd;
		return AmountToAdd - ActualAdd;
	}

	/** Removes items from the stack. Clears the slot if stack reaches zero. Returns actual amount removed. */
	int32 RemoveFromStack(int32 AmountToRemove)
	{
		if (AmountToRemove <= 0)
		{
			return 0;
		}

		const int32 ActualRemove = FMath::Min(AmountToRemove, CurrentStackSize);
		CurrentStackSize -= ActualRemove;

		if (CurrentStackSize <= 0)
		{
			ClearSlot();
		}

		return ActualRemove;
	}

	int32 TransferTo(FInventorySlot& TargetSlot, int32 Amount)
	{
		if (Amount <= 0 || IsEmpty())
		{
			return 0;
		}

		if (!TargetSlot.IsEmpty() && !TargetSlot.CanStackItem(Item))
		{
			return 0;
		}

		const int32 MaxCanTransfer = FMath::Min(Amount, CurrentStackSize);
		const int32 TargetSpace = TargetSlot.IsEmpty() ? MaxStackSize : TargetSlot.GetAvailableSpace();
		const int32 ActualTransfer = FMath::Min(MaxCanTransfer, TargetSpace);

		if (ActualTransfer <= 0)
		{
			return 0;
		}

		if (TargetSlot.IsEmpty())
		{
			TargetSlot.SetItem(Item, ActualTransfer);
		}
		else
		{
			TargetSlot.AddToStack(ActualTransfer);
		}

		RemoveFromStack(ActualTransfer);

		return ActualTransfer;
	}

	FString GetDebugString() const
	{
		if (IsEmpty())
		{
			return TEXT("Empty Slot");
		}

		return FString::Printf(TEXT("[%s] %d/%d"),
			*Item->GetItemDefinition().GetItemName().ToString(),
			CurrentStackSize,
			MaxStackSize);
	}

	bool ValidateSlot(TArray<FString>& OutErrors) const
	{
		bool bIsValid = true;

		if (CurrentStackSize < 0)
		{
			OutErrors.Add(FString::Printf(TEXT("Negative stack size: %d"), CurrentStackSize));
			bIsValid = false;
		}

		if (CurrentStackSize > MaxStackSize)
		{
			OutErrors.Add(FString::Printf(TEXT("Stack size %d exceeds max %d"), CurrentStackSize, MaxStackSize));
			bIsValid = false;
		}

		if (Item && CurrentStackSize == 0)
		{
			OutErrors.Add(TEXT("Item exists but stack size is 0"));
			bIsValid = false;
		}

		if (!Item && CurrentStackSize > 0)
		{
			OutErrors.Add(TEXT("No item but stack size > 0"));
			bIsValid = false;
		}

		return bIsValid;
	}

	bool operator==(const FInventorySlot& Other) const
	{
		if (IsEmpty() && Other.IsEmpty())
		{
			return true;
		}

		if (IsEmpty() != Other.IsEmpty())
		{
			return false;
		}

		return Item == Other.Item && CurrentStackSize == Other.CurrentStackSize;
	}

	bool operator!=(const FInventorySlot& Other) const
	{
		return !(*this == Other);
	}
};
