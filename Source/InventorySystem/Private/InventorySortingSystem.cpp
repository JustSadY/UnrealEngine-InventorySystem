#include "InventorySortingSystem.h"

void UInventorySortingSystem::SortItems(TArray<UItemBase*>& Items, const FInventorySortConfig& Config)
{
	if (Items.Num() <= 1)
	{
		return;
	}

	Items.RemoveAll([](UItemBase* Item) { return !IsValid(Item); });

	Items.Sort([&Config](const UItemBase& A, const UItemBase& B)
	{
		bool bALessThanB = CompareItems(&A, &B, Config.PrimarySortType, Config.PrimarySortDirection);
		bool bBLessThanA = CompareItems(&B, &A, Config.PrimarySortType, Config.PrimarySortDirection);

		if (bALessThanB != bBLessThanA)
		{
			return bALessThanB;
		}

		return CompareItems(&A, &B, Config.SecondarySortType, Config.SecondarySortDirection);
	});
}

void UInventorySortingSystem::SortByName(TArray<UItemBase*>& Items, EInventorySortDirection Direction)
{
	FInventorySortConfig Config;
	Config.PrimarySortType = EInventorySortType::IST_Name;
	Config.PrimarySortDirection = Direction;
	SortItems(Items, Config);
}

void UInventorySortingSystem::SortByType(TArray<UItemBase*>& Items, EInventorySortDirection Direction)
{
	FInventorySortConfig Config;
	Config.PrimarySortType = EInventorySortType::IST_Type;
	Config.PrimarySortDirection = Direction;
	SortItems(Items, Config);
}

void UInventorySortingSystem::SortByStackSize(TArray<UItemBase*>& Items, EInventorySortDirection Direction)
{
	FInventorySortConfig Config;
	Config.PrimarySortType = EInventorySortType::IST_StackSize;
	Config.PrimarySortDirection = Direction;
	SortItems(Items, Config);
}

bool UInventorySortingSystem::CompareItems(const UItemBase* A, const UItemBase* B, EInventorySortType SortType, EInventorySortDirection Direction)
{
	if (!A || !B)
	{
		return A != nullptr;
	}

	bool bResult = false;

	switch (SortType)
	{
	case EInventorySortType::IST_Name:
		{
			FString NameA = A->GetItemDefinition().GetItemName().ToString();
			FString NameB = B->GetItemDefinition().GetItemName().ToString();
			bResult = NameA < NameB;
			break;
		}

	case EInventorySortType::IST_Type:
		{
			FString TypeA = A->GetItemDefinition().GetItemID();
			FString TypeB = B->GetItemDefinition().GetItemID();
			bResult = TypeA < TypeB;
			break;
		}

	case EInventorySortType::IST_Rarity:
		{
			int32 RarityA = GetRarityValue(A);
			int32 RarityB = GetRarityValue(B);
			bResult = RarityA < RarityB;
			break;
		}

	case EInventorySortType::IST_StackSize:
		{
			bResult = A->GetCurrentStackSize() < B->GetCurrentStackSize();
			break;
		}

	default:
		bResult = false;
		break;
	}

	if (Direction == EInventorySortDirection::ISD_Descending)
	{
		bResult = !bResult;
	}

	return bResult;
}

int32 UInventorySortingSystem::GetRarityValue(const UItemBase* Item)
{
	return 0;
}

void UInventorySortingSystem::SortWithPredicate(TArray<UItemBase*>& Items, FCustomItemSortPredicate Predicate)
{
	if (!Predicate.IsBound())
	{
		return;
	}

	Items.Sort([&](const UItemBase& A, const UItemBase& B)
	{
		return Predicate.Execute(&A, &B);
	});
}

