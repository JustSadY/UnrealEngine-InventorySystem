#pragma once

#include "CoreMinimal.h"
#include "Items/ItemBase.h"
#include "InventorySortingSystem.h"
#include "InventorySearchFilter.generated.h"

USTRUCT(BlueprintType)
struct FInventoryFilterCriteria
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	FString SearchText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	TArray<int32> TypeIDs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	TArray<EItemRarity> Rarities;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	bool bOnlyStackable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	int32 MinStackSize = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	int32 MaxStackSize = 9999;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	bool bCaseSensitive = false;

	/** When true, uses Levenshtein distance matching instead of exact substring search. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	bool bUseFuzzySearch = false;

	FInventoryFilterCriteria() = default;
};

USTRUCT(BlueprintType)
struct FInventorySearchResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Search")
	TObjectPtr<UItemBase> Item = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Search")
	float RelevanceScore = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Search")
	int32 GroupIndex = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Search")
	int32 SlotIndex = -1;
};

UCLASS(BlueprintType)
class INVENTORYSYSTEM_API UInventorySearchFilter : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory|Search")
	static TArray<FInventorySearchResult> SearchItems(const TArray<UItemBase*>& Items, const FInventoryFilterCriteria& Criteria);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Search")
	static TArray<UItemBase*> QuickSearch(const TArray<UItemBase*>& Items, const FString& SearchText, bool bCaseSensitive = false);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Search")
	static TArray<UItemBase*> FilterByType(const TArray<UItemBase*>& Items, const TArray<int32>& TypeIDs);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Search")
	static TArray<UItemBase*> FilterByRarity(const TArray<UItemBase*>& Items, const TArray<EItemRarity>& Rarities);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Search")
	static TArray<UItemBase*> FilterStackable(const TArray<UItemBase*>& Items);

	static bool MatchesCriteria(const UItemBase* Item, const FInventoryFilterCriteria& Criteria);
	static float CalculateRelevanceScore(const UItemBase* Item, const FString& SearchText);

	/** Returns true if Pattern can be found in Source within a Levenshtein distance threshold. */
	static bool FuzzyMatch(const FString& Source, const FString& Pattern, bool bCaseSensitive = false);

	static int32 LevenshteinDistance(const FString& A, const FString& B);

protected:
	static bool ContainsText(const FString& Source, const FString& Pattern, bool bCaseSensitive);
};
