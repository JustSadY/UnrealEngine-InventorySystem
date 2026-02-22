#pragma once

#include "CoreMinimal.h"
#include "Items/ItemBase.h"
#include "ItemPoolTypes.generated.h"

/**
 * Pool information for a specific item class
 */
USTRUCT()
struct FItemPool
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<UItemBase*> AvailableItems;

	UPROPERTY()
	TArray<UItemBase*> ActiveItems;

	UPROPERTY()
	int32 MaxPoolSize = 100;

	UPROPERTY()
	int32 PrewarmCount = 10;

	UPROPERTY()
	bool bStrictLimit = false;

	UPROPERTY()
	bool bAutoGrow = false;

	UPROPERTY()
	int32 HitCount = 0;

	UPROPERTY()
	int32 MissCount = 0;

	UPROPERTY()
	int32 ReturnCount = 0;

	UPROPERTY()
	int32 OverflowCount = 0;

	float GetHitRate() const
	{
		int32 Total = HitCount + MissCount;
		return Total > 0 ? (static_cast<float>(HitCount) / Total) * 100.0f : 0.0f;
	}

	void ResetStats()
	{
		HitCount = 0;
		MissCount = 0;
		ReturnCount = 0;
		OverflowCount = 0;
	}
};