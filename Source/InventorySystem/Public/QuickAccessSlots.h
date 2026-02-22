#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/ItemBase.h"
#include "Struct/InventoryOperationResult.h"
#include "QuickAccessSlots.generated.h"

USTRUCT(BlueprintType)
struct FQuickSlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Quick Slot")
	TObjectPtr<UItemBase> Item = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Quick Slot")
	int32 SourceGroupIndex = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Quick Slot")
	int32 SourceSlotIndex = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quick Slot")
	FName KeyBinding = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Quick Slot")
	bool bIsLocked = false;

	void Clear()
	{
		Item = nullptr;
		SourceGroupIndex = -1;
		SourceSlotIndex = -1;
	}

	bool IsValid() const
	{
		return Item != nullptr;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuickSlotChanged, int32, SlotIndex, UItemBase*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuickSlotUsed, int32, SlotIndex, UItemBase*, Item);

/** Hotbar component that provides quick access to inventory items by slot index or key binding. */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class INVENTORYSYSTEM_API UQuickAccessSlots : public UActorComponent
{
	GENERATED_BODY()

public:
	UQuickAccessSlots();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quick Slots", Replicated)
	TArray<FQuickSlot> QuickSlots;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quick Slots")
	int32 MaxQuickSlots = 10;

	/** When true, automatically clears a quick slot if its item is removed from the inventory. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quick Slots")
	bool bAutoClearOnItemRemoved = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quick Slots")
	bool bAllowDuplicates = false;

public:
	UPROPERTY(BlueprintAssignable, Category = "Quick Slots|Events")
	FOnQuickSlotChanged OnQuickSlotChanged;

	UPROPERTY(BlueprintAssignable, Category = "Quick Slots|Events")
	FOnQuickSlotUsed OnQuickSlotUsed;

public:
	UFUNCTION(BlueprintCallable, Category = "Quick Slots")
	FInventoryOperationResult AssignToQuickSlot(int32 SlotIndex, UItemBase* Item, int32 SourceGroupIndex = -1, int32 SourceSlotIndex = -1);

	UFUNCTION(BlueprintCallable, Category = "Quick Slots")
	FInventoryOperationResult ClearQuickSlot(int32 SlotIndex);

	UFUNCTION(BlueprintPure, Category = "Quick Slots")
	UItemBase* GetQuickSlotItem(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Quick Slots")
	FQuickSlot GetQuickSlot(int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Quick Slots")
	FInventoryOperationResult UseQuickSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Quick Slots")
	FInventoryOperationResult SwapQuickSlots(int32 SlotIndexA, int32 SlotIndexB);

	UFUNCTION(BlueprintPure, Category = "Quick Slots")
	int32 FindQuickSlot(UItemBase* Item) const;

	UFUNCTION(BlueprintPure, Category = "Quick Slots")
	bool IsInQuickSlot(UItemBase* Item) const;

	UFUNCTION(BlueprintCallable, Category = "Quick Slots")
	void LockQuickSlot(int32 SlotIndex, bool bLocked);

	UFUNCTION(BlueprintCallable, Category = "Quick Slots")
	void ClearAllQuickSlots();

	UFUNCTION(BlueprintCallable, Category = "Quick Slots")
	void SetQuickSlotKeyBinding(int32 SlotIndex, FName KeyBinding);

	UFUNCTION(BlueprintPure, Category = "Quick Slots")
	const TArray<FQuickSlot>& GetAllQuickSlots() const { return QuickSlots; }

	void InitializeQuickSlots();

protected:
	bool IsValidSlotIndex(int32 SlotIndex) const;

	UFUNCTION()
	void OnItemRemovedFromInventory(UItemBase* Item);
};
