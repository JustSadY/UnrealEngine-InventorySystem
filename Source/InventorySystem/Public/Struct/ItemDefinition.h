#pragma once

#include "CoreMinimal.h"
#include "InventorySystem.h"
#include "Engine/Texture2D.h"
#include "ItemDefinition.generated.h"

/**
 * Encapsulated structure for item information.
 * Data is kept private to ensure integrity and accessed via Getters.
 */
USTRUCT(BlueprintType)
struct FItemDefinition
{
	GENERATED_BODY()

private:
	UPROPERTY(EditDefaultsOnly, Category = "Item Data")
	FString ItemID;

	UPROPERTY(EditDefaultsOnly, Category = "Item Data")
	FText ItemName;

	UPROPERTY(EditDefaultsOnly, Category = "Item Data", meta = (MultiLine = true))
	FText ItemDescription;

	UPROPERTY(EditDefaultsOnly, Category = "Item Data")
	TSoftObjectPtr<UTexture2D> ItemIcon;

	UPROPERTY(EditDefaultsOnly, Category = "Item Data")
	TArray<int32> InventorySlotTypeIDs;

public:
	FItemDefinition()
		: ItemID(TEXT("None"))
		  , ItemName(FText::FromString("New Item"))
		  , ItemDescription(FText::GetEmpty())
		  , ItemIcon(nullptr) 
	{ 
		InventorySlotTypeIDs.Add(0); 
	}

	FORCEINLINE const FString& GetItemID() const { return ItemID; }
	FORCEINLINE const FText& GetItemName() const { return ItemName; }
	FORCEINLINE const FText& GetItemDescription() const { return ItemDescription; }
	FORCEINLINE TSoftObjectPtr<UTexture2D> GetItemIcon() const { return ItemIcon; }
	FORCEINLINE const TArray<int32>& GetInventorySlotTypeIDs() const { return InventorySlotTypeIDs; }


	/** Rejects empty strings and logs a warning. */
	void SetItemID(const FString& NewID)
	{ 
		if (!NewID.IsEmpty())
		{
			ItemID = NewID; 
		}
		else
		{
			UE_LOG(LogInventory, Warning, TEXT("ItemDefinition: Attempted to set empty ItemID"));
		}
	}

	/** Rejects empty text and logs a warning. */
	void SetItemName(const FText& NewName)
	{ 
		if (!NewName.IsEmpty())
		{
			ItemName = NewName;
		}
		else
		{
			UE_LOG(LogInventory, Warning, TEXT("ItemDefinition: Attempted to set empty ItemName"));
		}
	}

	void SetItemDescription(const FText& NewDescription)
	{ 
		ItemDescription = NewDescription; 
	}

	void SetItemIcon(const TSoftObjectPtr<UTexture2D>& NewIcon) 
	{ 
		ItemIcon = NewIcon; 
	}

	/**
	 * Replaces all inventory slot type IDs
	 * @param NewTypeIDs Array of valid type IDs
	 */
	void SetInventorySlotTypeIDs(const TArray<int32>& NewTypeIDs)
	{
		bool bAllValid = true;
		for (int32 ID : NewTypeIDs)
		{
			if (ID < 0)
			{
				UE_LOG(LogInventory, Error, TEXT("ItemDefinition: Invalid TypeID %d in array (must be >= 0)"), ID);
				bAllValid = false;
			}
		}

		if (bAllValid)
		{
			InventorySlotTypeIDs = NewTypeIDs;
		}
		else
		{
			UE_LOG(LogInventory, Error, TEXT("ItemDefinition: SetInventorySlotTypeIDs failed due to invalid IDs"));
		}
	}

	/**
	 * Adds a single inventory slot type ID
	 * @param NewTypeID Type ID to add (must be >= 0 and unique)
	 */
	void AddInventorySlotTypeID(int32 NewTypeID)
	{
		if (NewTypeID < 0)
		{
			UE_LOG(LogInventory, Error, TEXT("ItemDefinition: Invalid TypeID %d (must be >= 0)"), NewTypeID);
			return;
		}

			const int32 NumBefore = InventorySlotTypeIDs.Num();
		InventorySlotTypeIDs.AddUnique(NewTypeID);
		
		if (InventorySlotTypeIDs.Num() == NumBefore)
		{
			UE_LOG(LogInventory, Verbose, TEXT("ItemDefinition: TypeID %d already exists, skipped"), NewTypeID);
		}
	}

	/**
	 * Removes an inventory slot type ID
	 * @param TypeID Type ID to remove
	 * @return True if removed, false if not found
	 */
	bool RemoveInventorySlotTypeID(int32 TypeID)
	{
		const int32 Removed = InventorySlotTypeIDs.Remove(TypeID);
		
		if (Removed > 0)
		{
			UE_LOG(LogInventory, Verbose, TEXT("ItemDefinition: Removed TypeID %d"), TypeID);
			return true;
		}
		
		UE_LOG(LogInventory, Warning, TEXT("ItemDefinition: TypeID %d not found for removal"), TypeID);
		return false;
	}

	/**
	 * Checks if a specific type ID is supported
	 * @param TypeID Type ID to check
	 * @return True if the type ID exists
	 */
	FORCEINLINE bool HasInventorySlotTypeID(int32 TypeID) const
	{
		return InventorySlotTypeIDs.Contains(TypeID);
	}

	void ClearInventorySlotTypeIDs()
	{
		InventorySlotTypeIDs.Empty();
		UE_LOG(LogInventory, Verbose, TEXT("ItemDefinition: Cleared all TypeIDs"));
	}
	
	/**
	 * Validates the item definition
	 * @param OutErrors Array to store validation errors
	 * @return True if valid, false otherwise
	 */
	bool Validate(TArray<FString>& OutErrors) const
	{
		bool bIsValid = true;

		if (ItemID.IsEmpty())
		{
			OutErrors.Add(TEXT("ItemID is empty"));
			bIsValid = false;
		}

		if (ItemName.IsEmpty())
		{
			OutErrors.Add(TEXT("ItemName is empty"));
			bIsValid = false;
		}

		if (InventorySlotTypeIDs.Num() == 0)
		{
			OutErrors.Add(TEXT("No inventory slot types defined"));
			bIsValid = false;
		}

		for (int32 ID : InventorySlotTypeIDs)
		{
			if (ID < 0)
			{
				OutErrors.Add(FString::Printf(TEXT("Invalid TypeID: %d"), ID));
				bIsValid = false;
			}
		}

		return bIsValid;
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("ItemDefinition[ID=%s, Name=%s, Types=%d]"),
			*ItemID,
			*ItemName.ToString(),
			InventorySlotTypeIDs.Num());
	}

	bool operator==(const FItemDefinition& Other) const
	{
		return ItemID == Other.ItemID;
	}

	bool operator!=(const FItemDefinition& Other) const
	{
		return ItemID != Other.ItemID;
	}
};