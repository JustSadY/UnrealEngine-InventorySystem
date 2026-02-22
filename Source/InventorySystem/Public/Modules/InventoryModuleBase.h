#pragma once

#include "CoreMinimal.h"
#include "Items/ItemBase.h"
#include "InventoryModuleBase.generated.h"

class UInventoryComponent;

/**
 * Enhanced base class for inventory modules with event hooks and validation support.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class INVENTORYSYSTEM_API UInventoryModuleBase : public UObject
{
	GENERATED_BODY()

public:
	UInventoryModuleBase();

	/**
	 * Internal initialization after the module is created.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Module|Lifecycle")
	void InitializeModule();
	virtual void InitializeModule_Implementation();

	/**
	 * Called when the owner actor starts play.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Module|Lifecycle")
	void BeginPlay();
	virtual void BeginPlay_Implementation();

	/**
	 * Frame-based update called by the inventory component.
	 * @param DeltaTime Seconds elapsed since last frame.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Module|Lifecycle")
	void Tick(float DeltaTime);
	virtual void Tick_Implementation(float DeltaTime);

	/**
	 * Logic executed when the module is linked to an inventory.
	 * @param ParentInventoryComponent The component this module is attached to.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Module|Events")
	void OnModuleInstalled(UInventoryComponent* ParentInventoryComponent);
	virtual void OnModuleInstalled_Implementation(UInventoryComponent* ParentInventoryComponent);

	/** Called when this module is detached from an inventory. */
	UFUNCTION(BlueprintNativeEvent, Category = "Module|Events")
	void OnModuleRemoved();
	virtual void OnModuleRemoved_Implementation();

	/**
	 * Reaction to an item being added to the parent inventory.
	 * @param Item The item instance added.
	 * @param GroupIndex Group location.
	 * @param SlotIndex Slot location.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Module|Inventory Events")
	void OnItemAdded(UItemBase* Item, int32 GroupIndex, int32 SlotIndex);

	virtual void OnItemAdded_Implementation(UItemBase* Item, int32 GroupIndex, int32 SlotIndex)
	{
	}

	/**
	 * Reaction to an item being removed from the parent inventory.
	 * @param Item The item instance removed.
	 * @param GroupIndex Group location.
	 * @param SlotIndex Slot location.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Module|Inventory Events")
	void OnItemRemoved(UItemBase* Item, int32 GroupIndex, int32 SlotIndex);

	virtual void OnItemRemoved_Implementation(UItemBase* Item, int32 GroupIndex, int32 SlotIndex)
	{
	}

	/**
	 * Validates if an inventory action is allowed by this module's logic.
	 * @param Item The item involved in the action.
	 * @param SlotTypeID The targeted slot type ID.
	 * @return True if the action is permitted.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Module|Validation")
	bool CanPerformAction(UItemBase* Item, int32 SlotTypeID);
	virtual bool CanPerformAction_Implementation(UItemBase* Item, int32 SlotTypeID) { return true; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Module Configuration")
	FName ModuleType;

	UPROPERTY(BlueprintReadOnly, Category = "Module")
	TObjectPtr<UInventoryComponent> OwningInventory;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Module Configuration")
	bool bWantsTick;

public:
	UFUNCTION(BlueprintCallable, Category = "Module")
	FName GetModuleType() const { return ModuleType; }

	UFUNCTION(BlueprintCallable, Category = "Module")
	UInventoryComponent* GetOwningInventory() const { return OwningInventory; }

	bool GetWantsTick() const { return bWantsTick; }

	virtual bool IsSupportedForNetworking() const override { return true; }
};
