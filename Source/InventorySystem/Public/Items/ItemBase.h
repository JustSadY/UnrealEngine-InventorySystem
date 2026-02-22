#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Modules/ItemModuleBase.h"
#include "Struct/ItemDefinition.h"
#include "Struct/InventoryOperationResult.h"
#include "Types/ItemSaveData.h"
#include "ItemBase.generated.h"

class UTexture2D;
class UInventoryComponent;

/**
 * Foundational class for all inventory items.
 * Uses a module-based approach via ItemModules for extensibility.
 */
UCLASS(Blueprintable, Abstract)
class INVENTORYSYSTEM_API UItemBase : public UObject
{
	GENERATED_BODY()

public:
	UItemBase();

	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual UWorld* GetWorld() const override;

	UFUNCTION(BlueprintCallable, Category = "Item|Save")
	virtual FItemSaveData SaveToStruct(bool bCompress = false);

	UFUNCTION(BlueprintCallable, Category = "Item|Save")
	virtual void LoadFromStruct(const FItemSaveData& Data);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item|Lifecycle")
	void InitializeItem();
	virtual void InitializeItem_Implementation();

	UFUNCTION(BlueprintPure, Category = "Item|Capabilities")
	bool IsStackable() const { return bIsStackable; }

	UFUNCTION(BlueprintPure, Category = "Item|Stacking")
	int32 GetCurrentStackSize() const { return CurrentStackSize; }

	UFUNCTION(BlueprintPure, Category = "Item|Stacking")
	int32 GetMaxStackSize() const { return MaxStackSize; }

	UFUNCTION(BlueprintCallable, Category = "Item|Stacking")
	void SetCurrentStackSize(int32 NewSize);

	UFUNCTION(BlueprintPure, Category = "Item|Stacking")
	bool CanMergeWith(const UItemBase* OtherItem) const;

	UFUNCTION(BlueprintCallable, Category = "Item|Stacking")
	FInventoryOperationResult MergeWith(UItemBase* OtherItem);

	/**
	 * Splits the stack, returning a new item with the specified amount.
	 * Caller is responsible for adding the returned item to an inventory.
	 */
	UFUNCTION(BlueprintCallable, Category = "Item|Stacking")
	UItemBase* SplitStack(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Item|State")
	virtual void OnAddedToInventory(AActor* NewOwner);

	UFUNCTION(BlueprintCallable, Category = "Item|State")
	virtual void OnRemovedFromInventory();

	UFUNCTION(BlueprintPure, Category = "Item|State")
	bool IsInInventory() const { return bIsInInventory; }

	UFUNCTION(BlueprintPure, Category = "Item|State")
	AActor* GetOwner() const { return OwnerActor; }

	UFUNCTION(BlueprintPure, Category = "Item|State")
	UInventoryComponent* GetInventoryComponent() const { return OwnerInventoryComponent; }

	UFUNCTION(BlueprintPure, Category = "Item|Definition")
	const FItemDefinition& GetItemDefinition() const { return ItemDefinition; }

	FItemDefinition& GetMutableItemDefinition() { return ItemDefinition; }

	UFUNCTION(BlueprintCallable, Category = "Item|Modules")
	virtual FInventoryOperationResult AddModule(UItemModuleBase* NewModule);

	UFUNCTION(BlueprintCallable, Category = "Item|Modules")
	virtual FInventoryOperationResult RemoveModule(UItemModuleBase* ModuleToRemove);

	UFUNCTION(BlueprintPure, Category = "Item|Modules")
	UItemModuleBase* GetModuleByClass(TSubclassOf<UItemModuleBase> ModuleClass) const;

	UFUNCTION(BlueprintPure, Category = "Item|Modules")
	TArray<UItemModuleBase*> GetAllModules() const { return ItemModules; }

	template <typename T>
	T* GetModule() const
	{
		for (int32 i = 0; i < ItemModules.Num(); ++i)
		{
			if (UItemModuleBase* Module = ItemModules[i])
			{
				if (T* CastedModule = Cast<T>(Module))
				{
					return CastedModule;
				}
			}
		}
		return nullptr;
	}

	/** Checks the module cache before doing a linear search. Prefer over GetModule for repeated access. */
	template <typename T>
	T* GetModuleCached() const
	{
		UClass* ClassToFind = T::StaticClass();

		if (UItemModuleBase* const* CachedModule = ModuleCache.Find(ClassToFind))
		{
			return Cast<T>(*CachedModule);
		}

		for (UItemModuleBase* Module : ItemModules)
		{
			if (Module && Module->IsA(ClassToFind))
			{
				ModuleCache.Add(ClassToFind, Module);
				return Cast<T>(Module);
			}
		}

		return nullptr;
	}

	/** Must be called whenever modules are added or removed to keep the cache valid. */
	void InvalidateModuleCache() const
	{
		ModuleCache.Empty();
	}

	UFUNCTION(BlueprintCallable, Category = "Item|Validation")
	bool Validate(TArray<FString>& OutErrors) const;

	UFUNCTION(BlueprintPure, Category = "Item|Debug")
	FString GetDebugString() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Data", meta = (AllowPrivateAccess = "true"))
	FItemDefinition ItemDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Stacking")
	bool bIsStackable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Stacking",
		meta = (EditCondition = "bIsStackable", ClampMin = "1"))
	int32 MaxStackSize;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Stacking", Replicated)
	int32 CurrentStackSize;

	UPROPERTY(BlueprintReadOnly, Category = "Item|State", Replicated)
	TObjectPtr<AActor> OwnerActor;

	UPROPERTY(BlueprintReadOnly, Category = "Item|State", Replicated)
	TObjectPtr<UInventoryComponent> OwnerInventoryComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Item|State", Replicated)
	bool bIsInInventory;

	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category = "Item|Modules", Replicated)
	TArray<TObjectPtr<UItemModuleBase>> ItemModules;

	mutable TMap<UClass*, UItemModuleBase*> ModuleCache;

	FString GenerateUniqueItemID() const;
	void CopyDefinitionTo(UItemBase* TargetItem) const;

	bool operator==(const UItemBase& other) const
	{
		return ItemDefinition == other.ItemDefinition;
	}

	bool operator!=(const UItemBase& other) const
	{
		return ItemDefinition != other.ItemDefinition;
	}
};
