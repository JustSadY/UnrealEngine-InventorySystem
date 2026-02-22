#pragma once

#include "CoreMinimal.h"
#include "ItemSaveData.generated.h"

class UItemBase;

USTRUCT(BlueprintType)
struct FItemSaveData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "SaveData")
	FString ItemID;

	UPROPERTY(BlueprintReadWrite, Category = "SaveData")
	TSubclassOf<UItemBase> ItemClass;

	UPROPERTY(BlueprintReadWrite, Category = "SaveData")
	int32 StackSize;

	UPROPERTY(BlueprintReadWrite, Category = "SaveData")
	TArray<uint8> ByteData;

	FItemSaveData()
		: StackSize(1)
	{
	}
};
