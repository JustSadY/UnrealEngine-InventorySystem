// InventorySearchFilter.cpp
#include "InventorySearchFilter.h"

TArray<FInventorySearchResult> UInventorySearchFilter::SearchItems(const TArray<UItemBase*>& Items,
                                                                   const FInventoryFilterCriteria& Criteria)
{
	TArray<FInventorySearchResult> Results;

	for (int32 i = 0; i < Items.Num(); ++i)
	{
		UItemBase* Item = Items[i];
		if (!IsValid(Item))
		{
			continue;
		}

		if (MatchesCriteria(Item, Criteria))
		{
			FInventorySearchResult Result;
			Result.Item = Item;
			Result.SlotIndex = i;
			Result.RelevanceScore = CalculateRelevanceScore(Item, Criteria.SearchText);
			Results.Add(Result);
		}
	}

	// Sort by relevance score (highest first)
	Results.Sort([](const FInventorySearchResult& A, const FInventorySearchResult& B)
	{
		return A.RelevanceScore > B.RelevanceScore;
	});

	return Results;
}

TArray<UItemBase*> UInventorySearchFilter::QuickSearch(const TArray<UItemBase*>& Items, const FString& SearchText,
                                                       bool bCaseSensitive)
{
	TArray<UItemBase*> Results;

	if (SearchText.IsEmpty())
	{
		return Items;
	}

	for (UItemBase* Item : Items)
	{
		if (!IsValid(Item))
		{
			continue;
		}

		FString ItemName = Item->GetItemDefinition().GetItemName().ToString();
		FString ItemDesc = Item->GetItemDefinition().GetItemDescription().ToString();

		if (ContainsText(ItemName, SearchText, bCaseSensitive) ||
			ContainsText(ItemDesc, SearchText, bCaseSensitive))
		{
			Results.Add(Item);
		}
	}

	return Results;
}

TArray<UItemBase*> UInventorySearchFilter::FilterByType(const TArray<UItemBase*>& Items, const TArray<int32>& TypeIDs)
{
	TArray<UItemBase*> Results;

	if (TypeIDs.Num() == 0)
	{
		return Items;
	}

	for (UItemBase* Item : Items)
	{
		if (!IsValid(Item))
		{
			continue;
		}

		const TArray<int32>& ItemTypes = Item->GetItemDefinition().GetInventorySlotTypeIDs();

		for (int32 TypeID : TypeIDs)
		{
			if (ItemTypes.Contains(TypeID))
			{
				Results.Add(Item);
				break;
			}
		}
	}

	return Results;
}

TArray<UItemBase*> UInventorySearchFilter::FilterByRarity(const TArray<UItemBase*>& Items,
                                                          const TArray<EItemRarity>& Rarities)
{
	TArray<UItemBase*> Results;

	if (Rarities.Num() == 0)
	{
		return Items;
	}

	for (UItemBase* Item : Items)
	{
		if (!IsValid(Item))
		{
			continue;
		}

		// Get rarity from item - this would need to be implemented in your item system
		int32 RarityValue = UInventorySortingSystem::GetRarityValue(Item);
		EItemRarity ItemRarity = static_cast<EItemRarity>(RarityValue);

		if (Rarities.Contains(ItemRarity))
		{
			Results.Add(Item);
		}
	}

	return Results;
}

TArray<UItemBase*> UInventorySearchFilter::FilterStackable(const TArray<UItemBase*>& Items)
{
	TArray<UItemBase*> Results;

	for (UItemBase* Item : Items)
	{
		if (IsValid(Item) && Item->IsStackable())
		{
			Results.Add(Item);
		}
	}

	return Results;
}

bool UInventorySearchFilter::MatchesCriteria(const UItemBase* Item, const FInventoryFilterCriteria& Criteria)
{
	if (!IsValid(Item))
	{
		return false;
	}

	// Text search
	if (!Criteria.SearchText.IsEmpty())
	{
		FString ItemName = Item->GetItemDefinition().GetItemName().ToString();
		FString ItemDesc = Item->GetItemDefinition().GetItemDescription().ToString();

		bool bTextMatch = false;

		if (Criteria.bUseFuzzySearch)
		{
			bTextMatch = FuzzyMatch(ItemName, Criteria.SearchText, Criteria.bCaseSensitive) ||
				FuzzyMatch(ItemDesc, Criteria.SearchText, Criteria.bCaseSensitive);
		}
		else
		{
			bTextMatch = ContainsText(ItemName, Criteria.SearchText, Criteria.bCaseSensitive) ||
				ContainsText(ItemDesc, Criteria.SearchText, Criteria.bCaseSensitive);
		}

		if (!bTextMatch)
		{
			return false;
		}
	}

	// Type filter
	if (Criteria.TypeIDs.Num() > 0)
	{
		const TArray<int32>& ItemTypes = Item->GetItemDefinition().GetInventorySlotTypeIDs();
		bool bTypeMatch = false;

		for (int32 TypeID : Criteria.TypeIDs)
		{
			if (ItemTypes.Contains(TypeID))
			{
				bTypeMatch = true;
				break;
			}
		}

		if (!bTypeMatch)
		{
			return false;
		}
	}

	// Rarity filter
	if (Criteria.Rarities.Num() > 0)
	{
		int32 RarityValue = UInventorySortingSystem::GetRarityValue(Item);
		EItemRarity ItemRarity = static_cast<EItemRarity>(RarityValue);

		if (!Criteria.Rarities.Contains(ItemRarity))
		{
			return false;
		}
	}

	// Stackable filter
	if (Criteria.bOnlyStackable && !Item->IsStackable())
	{
		return false;
	}

	// Stack size filter
	int32 StackSize = Item->GetCurrentStackSize();
	if (StackSize < Criteria.MinStackSize || StackSize > Criteria.MaxStackSize)
	{
		return false;
	}

	return true;
}

float UInventorySearchFilter::CalculateRelevanceScore(const UItemBase* Item, const FString& SearchText)
{
	if (!IsValid(Item) || SearchText.IsEmpty())
	{
		return 0.0f;
	}

	float Score = 0.0f;
	FString ItemName = Item->GetItemDefinition().GetItemName().ToString();
	FString ItemDesc = Item->GetItemDefinition().GetItemDescription().ToString();

	// Exact match in name = highest score
	if (ItemName.Equals(SearchText, ESearchCase::IgnoreCase))
	{
		Score += 100.0f;
	}
	// Starts with search text = high score
	else if (ItemName.StartsWith(SearchText, ESearchCase::IgnoreCase))
	{
		Score += 75.0f;
	}
	// Contains search text = medium score
	else if (ItemName.Contains(SearchText, ESearchCase::IgnoreCase))
	{
		Score += 50.0f;
	}

	// Description match = lower score
	if (ItemDesc.Contains(SearchText, ESearchCase::IgnoreCase))
	{
		Score += 25.0f;
	}

	// Bonus for shorter names (more specific match)
	if (ItemName.Len() > 0)
	{
		Score += (100.0f - ItemName.Len()) * 0.1f;
	}

	// Levenshtein distance bonus (similarity)
	int32 Distance = LevenshteinDistance(ItemName.ToLower(), SearchText.ToLower());
	Score += FMath::Max(0.0f, 20.0f - Distance);

	return Score;
}

bool UInventorySearchFilter::FuzzyMatch(const FString& Source, const FString& Pattern, bool bCaseSensitive)
{
	if (Pattern.IsEmpty())
	{
		return true;
	}

	if (Source.IsEmpty())
	{
		return false;
	}

	FString SourceStr = bCaseSensitive ? Source : Source.ToLower();
	FString PatternStr = bCaseSensitive ? Pattern : Pattern.ToLower();

	int32 PatternIdx = 0;
	int32 SourceIdx = 0;

	while (SourceIdx < SourceStr.Len() && PatternIdx < PatternStr.Len())
	{
		if (SourceStr[SourceIdx] == PatternStr[PatternIdx])
		{
			PatternIdx++;
		}
		SourceIdx++;
	}

	return PatternIdx == PatternStr.Len();
}

int32 UInventorySearchFilter::LevenshteinDistance(const FString& A, const FString& B)
{
	const int32 LenA = A.Len();
	const int32 LenB = B.Len();

	if (LenA == 0) return LenB;
	if (LenB == 0) return LenA;

	TArray<int32> PrevRow;
	TArray<int32> CurrRow;

	PrevRow.SetNum(LenB + 1);
	CurrRow.SetNum(LenB + 1);

	for (int32 j = 0; j <= LenB; ++j)
	{
		PrevRow[j] = j;
	}

	for (int32 i = 1; i <= LenA; ++i)
	{
		CurrRow[0] = i;

		for (int32 j = 1; j <= LenB; ++j)
		{
			int32 Cost = (A[i - 1] == B[j - 1]) ? 0 : 1;
			CurrRow[j] = FMath::Min3(
				PrevRow[j] + 1, // Deletion
				CurrRow[j - 1] + 1, // Insertion
				PrevRow[j - 1] + Cost // Substitution
			);
		}

		Swap(PrevRow, CurrRow);
	}

	return PrevRow[LenB];
}

bool UInventorySearchFilter::ContainsText(const FString& Source, const FString& Pattern, bool bCaseSensitive)
{
	if (bCaseSensitive)
	{
		return Source.Contains(Pattern, ESearchCase::CaseSensitive);
	}
	else
	{
		return Source.Contains(Pattern, ESearchCase::IgnoreCase);
	}
}
