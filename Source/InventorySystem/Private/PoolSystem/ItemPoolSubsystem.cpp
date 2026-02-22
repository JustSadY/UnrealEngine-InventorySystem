#include "PoolSystem/ItemPoolSubsystem.h"

#include "InventorySystem.h"
#include "Engine/Engine.h"

void UItemPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogInventory, Log, TEXT("ItemPoolSubsystem initialized"));
}

void UItemPoolSubsystem::Deinitialize()
{
	ClearAllPools();
	Super::Deinitialize();
	UE_LOG(LogInventory, Log, TEXT("ItemPoolSubsystem deinitialized"));
}

UItemBase* UItemPoolSubsystem::GetItemFromPool(TSubclassOf<UItemBase> ItemClass, UObject* Outer)
{
	if (!ItemClass || !bEnablePooling)
	{
		if (ItemClass && Outer)
		{
			UItemBase* NewItem = NewObject<UItemBase>(Outer, ItemClass);
			if (NewItem)
			{
				NewItem->InitializeItem();
			}
			return NewItem;
		}
		return nullptr;
	}

	FItemPool* Pool = ItemPools.Find(ItemClass);
	if (!Pool)
	{
		CreatePool(ItemClass);
		Pool = ItemPools.Find(ItemClass);
	}

	if (!Pool)
	{
		UE_LOG(LogInventory, Error, TEXT("Failed to create pool for item class"));
		return nullptr;
	}

	UItemBase* Item = nullptr;

	if (Pool->AvailableItems.Num() > 0)
	{
		Item = Pool->AvailableItems.Pop();

		if (Item)
		{
			Pool->HitCount++;

			if (Outer)
			{
				Item->Rename(nullptr, Outer);
			}

			ResetItem(Item);
			Item->InitializeItem();
		}
	}
	else if (UEngineItemPoolSubsystem* EnginePool = GEngine->GetEngineSubsystem<UEngineItemPoolSubsystem>())
	{
		Item = EnginePool->GetItemFromPool(ItemClass, Outer);
	}

	if (!Item)
	{
		Pool->MissCount++;

		if (Pool->bStrictLimit)
		{
			UE_LOG(LogInventory, Warning, TEXT("Pool empty for %s and Strict Limit is ON. Returning nullptr."),
			       *ItemClass->GetName());
			return nullptr;
		}

		if (Outer)
		{
			Item = NewObject<UItemBase>(Outer, ItemClass);
			if (Item)
			{
				Item->InitializeItem();
			}
		}
	}

	if (Item)
	{
		Pool->ActiveItems.Add(Item);
	}

	return Item;
}

void UItemPoolSubsystem::ReturnItemToPool(UItemBase* Item)
{
	if (!Item || !bEnablePooling)
	{
		return;
	}

	TSubclassOf<UItemBase> ItemClass = Item->GetClass();
	FItemPool* Pool = ItemPools.Find(ItemClass);

	if (!Pool)
	{
		CreatePool(ItemClass);
		Pool = ItemPools.Find(ItemClass);
	}

	if (!Pool)
	{
		return;
	}

	Pool->ActiveItems.Remove(Item);

	if (Pool->AvailableItems.Num() < Pool->MaxPoolSize)
	{
		Pool->ReturnCount++;
		ResetItem(Item);

		Item->Rename(nullptr, this);

		Pool->AvailableItems.Add(Item);
	}
	else
	{
		if (Pool->bAutoGrow)
		{
			Pool->ReturnCount++;
			Pool->MaxPoolSize++;
			ResetItem(Item);
			Item->Rename(nullptr, this);
			Pool->AvailableItems.Add(Item);
			UE_LOG(LogInventory, Verbose, TEXT("Pool for %s auto-grew to size %d"), *ItemClass->GetName(),
			       Pool->MaxPoolSize);
		}
		else
		{
			Pool->OverflowCount++;
			if (UEngineItemPoolSubsystem* EnginePool = GEngine->GetEngineSubsystem<UEngineItemPoolSubsystem>())
			{
				EnginePool->ReturnItemToPool(Item);
			}
			else
			{
				Item->MarkAsGarbage();
			}
		}
	}
}

void UItemPoolSubsystem::PrewarmPool(TSubclassOf<UItemBase> ItemClass, int32 Count)
{
	if (!ItemClass || Count <= 0)
	{
		return;
	}

	FItemPool* Pool = ItemPools.Find(ItemClass);
	if (!Pool)
	{
		CreatePool(ItemClass);
		Pool = ItemPools.Find(ItemClass);
	}

	if (!Pool)
	{
		return;
	}

	for (int32 i = 0; i < Count; ++i)
	{
		UItemBase* NewItem = NewObject<UItemBase>(this, ItemClass);
		if (NewItem)
		{
			NewItem->InitializeItem();
			ResetItem(NewItem);
			Pool->AvailableItems.Add(NewItem);
		}
	}

	UE_LOG(LogInventory, Log, TEXT("Prewarmed pool for %s with %d items"), *ItemClass->GetName(), Count);
}

void UItemPoolSubsystem::ClearAllPools()
{
	for (auto& Pair : ItemPools)
	{
		FItemPool& Pool = Pair.Value;

		for (UItemBase* Item : Pool.AvailableItems)
		{
			if (Item)
			{
				Item->MarkAsGarbage();
			}
		}

		for (UItemBase* Item : Pool.ActiveItems)
		{
			if (Item)
			{
				Item->MarkAsGarbage();
			}
		}

		Pool.AvailableItems.Empty();
		Pool.ActiveItems.Empty();
	}

	ItemPools.Empty();
	UE_LOG(LogInventory, Log, TEXT("Cleared all item pools"));
}

void UItemPoolSubsystem::ClearPool(TSubclassOf<UItemBase> ItemClass)
{
	if (!ItemClass)
	{
		return;
	}

	FItemPool* Pool = ItemPools.Find(ItemClass);
	if (Pool)
	{
		for (UItemBase* Item : Pool->AvailableItems)
		{
			if (Item)
			{
				Item->MarkAsGarbage();
			}
		}

		Pool->AvailableItems.Empty();
		UE_LOG(LogInventory, Log, TEXT("Cleared pool for %s"), *ItemClass->GetName());
	}
}

void UItemPoolSubsystem::GetPoolStats(TSubclassOf<UItemBase> ItemClass, int32& OutAvailable, int32& OutActive,
                                      int32& OutTotal)
{
	OutAvailable = 0;
	OutActive = 0;
	OutTotal = 0;

	if (!ItemClass)
	{
		return;
	}

	FItemPool* Pool = ItemPools.Find(ItemClass);
	if (Pool)
	{
		OutAvailable = Pool->AvailableItems.Num();
		OutActive = Pool->ActiveItems.Num();
		OutTotal = OutAvailable + OutActive;
	}
}

void UItemPoolSubsystem::SetMaxPoolSize(TSubclassOf<UItemBase> ItemClass, int32 MaxSize)
{
	if (!ItemClass || MaxSize < 0)
	{
		return;
	}

	FItemPool* Pool = ItemPools.Find(ItemClass);
	if (!Pool)
	{
		CreatePool(ItemClass);
		Pool = ItemPools.Find(ItemClass);
	}

	if (Pool)
	{
		Pool->MaxPoolSize = MaxSize;

		while (Pool->AvailableItems.Num() > MaxSize)
		{
			UItemBase* Item = Pool->AvailableItems.Pop();
			if (Item)
			{
				if (UEngineItemPoolSubsystem* EnginePool = GEngine->GetEngineSubsystem<UEngineItemPoolSubsystem>())
				{
					EnginePool->ReturnItemToPool(Item);
				}
				else
				{
					Item->MarkAsGarbage();
				}
			}
		}
	}
}

void UItemPoolSubsystem::ConfigurePool(TSubclassOf<UItemBase> ItemClass, bool bStrictLimit, bool bAutoGrow)
{
	if (!ItemClass)
	{
		return;
	}

	FItemPool* Pool = ItemPools.Find(ItemClass);
	if (!Pool)
	{
		CreatePool(ItemClass);
		Pool = ItemPools.Find(ItemClass);
	}

	if (Pool)
	{
		Pool->bStrictLimit = bStrictLimit;
		Pool->bAutoGrow = bAutoGrow;
	}
}

void UItemPoolSubsystem::CreatePool(TSubclassOf<UItemBase> ItemClass)
{
	if (!ItemClass)
	{
		return;
	}

	FItemPool NewPool;
	NewPool.MaxPoolSize = DefaultMaxPoolSize;
	NewPool.PrewarmCount = DefaultPrewarmCount;
	NewPool.bStrictLimit = bDefaultStrictLimit;
	NewPool.bAutoGrow = bDefaultAutoGrow;

	ItemPools.Add(ItemClass, NewPool);

	PrewarmPool(ItemClass, NewPool.PrewarmCount);
}

void UItemPoolSubsystem::ResetItem(UItemBase* Item)
{
	if (!Item)
	{
		return;
	}

	Item->SetCurrentStackSize(1);
	Item->OnRemovedFromInventory();

	for (UItemModuleBase* Module : Item->GetAllModules())
	{
		if (Module)
		{
			Module->Reset();
		}
	}
}

float UItemPoolSubsystem::GetPoolHitRate(TSubclassOf<UItemBase> ItemClass)
{
	if (!ItemClass)
	{
		return 0.0f;
	}

	FItemPool* Pool = ItemPools.Find(ItemClass);
	return Pool ? Pool->GetHitRate() : 0.0f;
}

FString UItemPoolSubsystem::GetAllPoolStatsSummary()
{
	FString Summary = TEXT("=== World Pool Stats ===\n");

	for (auto& Pair : ItemPools)
	{
		const FItemPool& Pool = Pair.Value;
		Summary += FString::Printf(
			TEXT("  [%s] Available: %d | Active: %d | Hit: %d | Miss: %d | Rate: %.1f%% | Return: %d | Overflow: %d\n"),
			*Pair.Key->GetName(),
			Pool.AvailableItems.Num(),
			Pool.ActiveItems.Num(),
			Pool.HitCount,
			Pool.MissCount,
			Pool.GetHitRate(),
			Pool.ReturnCount,
			Pool.OverflowCount);
	}

	return Summary;
}

void UItemPoolSubsystem::ResetPoolStats()
{
	for (auto& Pair : ItemPools)
	{
		Pair.Value.ResetStats();
	}
	UE_LOG(LogInventory, Log, TEXT("World pool stats reset"));
}