#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Struct/InventorySlotsGroup.h"
#include "Struct/InventoryOperationResult.h"
#include "InventoryComponent.generated.h"

class UItemBase;
class UInventoryModuleBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnItemAdded, UItemBase*, Item, int32, GroupIndex, int32, SlotIndex);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnItemRemoved, UItemBase*, Item, int32, GroupIndex, int32, SlotIndex);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnItemStackChanged, UItemBase*, Item, int32, GroupIndex, int32,
                                              SlotIndex, int32, OldAmount, int32, NewAmount);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryFull, UItemBase*, Item, int32, RequiredSlots);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class INVENTORYSYSTEM_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch,
	                                 FReplicationFlags* RepFlags) override;

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", Replicated)
	FInventorySlotsGroup InventorySlotsGroup;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	bool bAutoStackItems;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Modules", Replicated)
	TArray<TObjectPtr<UInventoryModuleBase>> InstalledModules;

public:
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemAdded OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemRemoved OnItemRemoved;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemStackChanged OnItemStackChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryFull OnInventoryFull;

public:
	/**
	 * Attempts to add an item to a specific slot group, or any compatible group if TargetTypeID is -1.
	 * @param Item Item instance to add.
	 * @param TargetTypeID Target slot group type ID. Pass -1 to auto-select the first compatible group.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventoryOperationResult AddItem(UItemBase* Item, int32 TargetTypeID = -1);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventoryOperationResult RemoveItem(UItemBase* Item);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventoryOperationResult RemoveItemAt(int32 TypeID, int32 SlotIndex, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UItemBase* GetItemAtIndex(int32 SlotTypeID, int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool FindItemLocation(UItemBase* Item, int32& OutTypeID, int32& OutSlotIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventoryOperationResult TransferItem(int32 FromTypeID, int32 FromIndex, int32 ToTypeID, int32 ToIndex);

	/**
	 * Returns true if the item can be added to the inventory.
	 * @param SlotTypeID Restrict check to a specific slot group. Pass -1 to check all groups.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool CanAddItem(UItemBase* Item, int32 SlotTypeID = -1) const;

	/**
	 * Returns the number of empty slots. Pass -1 to count across all groups.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetEmptySlotCount(int32 SlotTypeID = -1) const;

	/** Returns how many units of an item exist in the inventory. Pass -1 to count across all groups. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetItemCount(UItemBase* Item, int32 SlotTypeID = -1) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<UItemBase*> GetAllItems() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SortInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void OrganizeInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearInventory();

	/**
	 * Attempts to merge the item into existing stacks without occupying a new slot.
	 * @param SlotTypeID Restrict stacking to a specific slot group. Pass -1 to search all groups.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Stacking")
	FInventoryOperationResult TryStackItem(UItemBase* Item, int32 SlotTypeID = -1);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventoryOperationResult AddItemByClass(TSubclassOf<UItemBase> ItemClass, int32 Quantity = 1, int32 SlotTypeID = -1);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UItemBase* CreateItemInstance(TSubclassOf<UItemBase> ItemClass);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Modules")
	FInventoryOperationResult InstallModule(UInventoryModuleBase* Module);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Modules")
	FInventoryOperationResult InstallModuleByClass(TSubclassOf<UInventoryModuleBase> ModuleClass);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Modules")
	FInventoryOperationResult RemoveModule(UInventoryModuleBase* Module);

	template <typename T>
	T* GetModule() const
	{
		for (UInventoryModuleBase* Module : InstalledModules)
		{
			if (T* TypedModule = Cast<T>(Module))
			{
				return TypedModule;
			}
		}
		return nullptr;
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventorySlotsGroup& GetInventorySlotsGroup() { return InventorySlotsGroup; }

	const FInventorySlotsGroup& GetInventorySlotsGroup() const { return InventorySlotsGroup; }
};
