#pragma once

#include "CoreMinimal.h"
#include "InventoryOperationResult.generated.h"

USTRUCT(BlueprintType)
struct FInventoryOperationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool bSuccess;

	UPROPERTY(BlueprintReadOnly)
	FString Message;

	FORCEINLINE operator bool() const
	{
		return bSuccess;
	}

	static FInventoryOperationResult Ok()
	{
		FInventoryOperationResult Result;
		Result.bSuccess = true;
		Result.Message = TEXT("Success");
		return Result;
	}

	static FInventoryOperationResult Fail(const FString& Reason)
	{
		FInventoryOperationResult Result;
		Result.bSuccess = false;
		Result.Message = Reason;
		return Result;
	}

	FInventoryOperationResult()
		: bSuccess(false), Message(TEXT(""))
	{
	}
};