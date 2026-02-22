#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Items/ItemBase.h"
#include "PoolSystem/ItemPoolTypes.h"
#include "PoolSystem/EngineItemPoolSubsystem.h"
#include "ItemPoolSubsystem.generated.h"

/**
 * World subsystem for managing item object pooling
 * Reduces garbage collection pressure and improves performance
 */
UCLASS()
class INVENTORYSYSTEM_API UItemPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

protected:
	/** Map of item class to its pool */
	UPROPERTY()
	TMap<TSubclassOf<UItemBase>, FItemPool> ItemPools;

	/** Enable/disable pooling system */
	UPROPERTY(EditDefaultsOnly, Category = "Pool Settings")
	bool bEnablePooling = true;

	/** Default maximum pool size */
	UPROPERTY(EditDefaultsOnly, Category = "Pool Settings")
	int32 DefaultMaxPoolSize = 100;

	/** Default prewarm count */
	UPROPERTY(EditDefaultsOnly, Category = "Pool Settings")
	int32 DefaultPrewarmCount = 10;

	/** Default strict limit setting for new pools */
	UPROPERTY(EditDefaultsOnly, Category = "Pool Settings")
	bool bDefaultStrictLimit = false;

	/** Default auto grow setting for new pools */
	UPROPERTY(EditDefaultsOnly, Category = "Pool Settings")
	bool bDefaultAutoGrow = true;

public:
	/**
	 * Get or create an item from the pool
	 * @param ItemClass Class of the item to spawn
	 * @param Outer Outer object for the item
	 * @return Pooled or newly created item instance
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Pool")
	UItemBase* GetItemFromPool(TSubclassOf<UItemBase> ItemClass, UObject* Outer);

	/**
	 * Return an item to the pool
	 * @param Item Item to return to pool
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Pool")
	void ReturnItemToPool(UItemBase* Item);

	/**
	 * Prewarm a pool for a specific item class
	 * @param ItemClass Class to prewarm
	 * @param Count Number of items to create
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Pool")
	void PrewarmPool(TSubclassOf<UItemBase> ItemClass, int32 Count);

	/**
	 * Clear all pools
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Pool")
	void ClearAllPools();

	/**
	 * Clear a specific pool
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Pool")
	void ClearPool(TSubclassOf<UItemBase> ItemClass);

	/**
	 * Get pool statistics
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Pool")
	void GetPoolStats(TSubclassOf<UItemBase> ItemClass, int32& OutAvailable, int32& OutActive, int32& OutTotal);

	/**
	 * Set max pool size for a specific item class
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Pool")
	void SetMaxPoolSize(TSubclassOf<UItemBase> ItemClass, int32 MaxSize);

	/**
	 * Configure pool behavior settings
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Pool")
	void ConfigurePool(TSubclassOf<UItemBase> ItemClass, bool bStrictLimit, bool bAutoGrow);

	/**
	 * Get hit rate for a specific pool
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Pool|Stats")
	float GetPoolHitRate(TSubclassOf<UItemBase> ItemClass);

	/**
	 * Get a summary string of all pool stats
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Pool|Stats")
	FString GetAllPoolStatsSummary();

	/**
	 * Reset hit/miss stats for all pools
	 */
	UFUNCTION(BlueprintCallable, Category = "Item Pool|Stats")
	void ResetPoolStats();

protected:
	/**
	 * Create a new pool entry for an item class
	 */
	void CreatePool(TSubclassOf<UItemBase> ItemClass);

	/**
	 * Reset an item to default state before returning to pool
	 */
	void ResetItem(UItemBase* Item);
};