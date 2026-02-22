#pragma once

#include "CoreMinimal.h"
#include "Items/ItemBase.h"
#include "InventorySortingSystem.generated.h"

UENUM(BlueprintType)
enum class EInventorySortType : uint8
{
	IST_Name         UMETA(DisplayName = "Name"),
	IST_Type         UMETA(DisplayName = "Type"),
	IST_Rarity       UMETA(DisplayName = "Rarity"),
	IST_Value        UMETA(DisplayName = "Value"),
	IST_StackSize    UMETA(DisplayName = "Stack Size"),
	IST_Weight       UMETA(DisplayName = "Weight"),
	IST_Level        UMETA(DisplayName = "Level Requirement"),
	IST_Custom       UMETA(DisplayName = "Custom")
};

UENUM(BlueprintType)
enum class EInventorySortDirection : uint8
{
	ISD_Ascending    UMETA(DisplayName = "Ascending"),
	ISD_Descending   UMETA(DisplayName = "Descending")
};

UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	IR_Common        UMETA(DisplayName = "Common"),
	IR_Uncommon      UMETA(DisplayName = "Uncommon"),
	IR_Rare          UMETA(DisplayName = "Rare"),
	IR_Epic          UMETA(DisplayName = "Epic"),
	IR_Legendary     UMETA(DisplayName = "Legendary"),
	IR_Mythic        UMETA(DisplayName = "Mythic")
};

USTRUCT(BlueprintType)
struct FInventorySortConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sorting")
	EInventorySortType PrimarySortType = EInventorySortType::IST_Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sorting")
	EInventorySortDirection PrimarySortDirection = EInventorySortDirection::ISD_Ascending;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sorting")
	EInventorySortType SecondarySortType = EInventorySortType::IST_Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sorting")
	EInventorySortDirection SecondarySortDirection = EInventorySortDirection::ISD_Ascending;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sorting")
	bool bGroupByType = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sorting")
	bool bEmptySlotsAtEnd = true;
};

DECLARE_DELEGATE_RetVal_TwoParams(bool, FCustomItemSortPredicate, const UItemBase*, const UItemBase*);

UCLASS(BlueprintType)
class INVENTORYSYSTEM_API UInventorySortingSystem : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory|Sorting")
	static void SortItems(UPARAM(ref) TArray<UItemBase*>& Items, const FInventorySortConfig& Config);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Sorting")
	static void SortByName(UPARAM(ref) TArray<UItemBase*>& Items, EInventorySortDirection Direction = EInventorySortDirection::ISD_Ascending);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Sorting")
	static void SortByType(UPARAM(ref) TArray<UItemBase*>& Items, EInventorySortDirection Direction = EInventorySortDirection::ISD_Ascending);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Sorting")
	static void SortByStackSize(UPARAM(ref) TArray<UItemBase*>& Items, EInventorySortDirection Direction = EInventorySortDirection::ISD_Descending);

	static bool CompareItems(const UItemBase* A, const UItemBase* B, EInventorySortType SortType, EInventorySortDirection Direction);
	static int32 GetRarityValue(const UItemBase* Item);
	static void SortWithPredicate(TArray<UItemBase*>& Items, FCustomItemSortPredicate Predicate);
};
