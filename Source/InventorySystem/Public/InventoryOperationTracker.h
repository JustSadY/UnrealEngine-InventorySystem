#pragma once

#include "CoreMinimal.h"
#include "Struct/InventoryOperationResult.h"
#include "InventoryOperationTracker.generated.h"

/**
 * Types of inventory operations that can be tracked.
 */
UENUM(BlueprintType)
enum class EInventoryOperationType : uint8
{
	IOT_AddItem UMETA(DisplayName = "Add Item"),
	IOT_RemoveItem UMETA(DisplayName = "Remove Item"),
	IOT_RemoveItemAt UMETA(DisplayName = "Remove Item At"),
	IOT_TransferItem UMETA(DisplayName = "Transfer Item"),
	IOT_StackItem UMETA(DisplayName = "Stack Item"),
	IOT_SplitStack UMETA(DisplayName = "Split Stack"),
	IOT_SwapSlots UMETA(DisplayName = "Swap Slots"),
	IOT_InstallModule UMETA(DisplayName = "Install Module"),
	IOT_RemoveModule UMETA(DisplayName = "Remove Module"),
	IOT_QuickSlotAssign UMETA(DisplayName = "Quick Slot Assign"),
	IOT_QuickSlotClear UMETA(DisplayName = "Quick Slot Clear"),
	IOT_QuickSlotUse UMETA(DisplayName = "Quick Slot Use"),
	IOT_QuickSlotSwap UMETA(DisplayName = "Quick Slot Swap"),
	IOT_MergeItem UMETA(DisplayName = "Merge Item"),
	IOT_AddItemModule UMETA(DisplayName = "Add Item Module"),
	IOT_RemoveItemModule UMETA(DisplayName = "Remove Item Module"),
	IOT_Other UMETA(DisplayName = "Other")
};

/**
 * A single recorded inventory operation.
 */
USTRUCT(BlueprintType)
struct FInventoryOperationRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Operation Tracker")
	EInventoryOperationType OperationType = EInventoryOperationType::IOT_Other;

	UPROPERTY(BlueprintReadOnly, Category = "Operation Tracker")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly, Category = "Operation Tracker")
	FString Message;

	UPROPERTY(BlueprintReadOnly, Category = "Operation Tracker")
	float DurationMs = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Operation Tracker")
	FString ContextInfo;

	double Timestamp = 0.0;
};

/**
 * Aggregated statistics for a specific operation type.
 */
USTRUCT(BlueprintType)
struct FOperationTypeStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Operation Tracker")
	int32 TotalCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Operation Tracker")
	int32 SuccessCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Operation Tracker")
	int32 FailCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Operation Tracker")
	float TotalDurationMs = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Operation Tracker")
	float MinDurationMs = TNumericLimits<float>::Max();

	UPROPERTY(BlueprintReadOnly, Category = "Operation Tracker")
	float MaxDurationMs = 0.0f;

	float GetAverageDurationMs() const
	{
		return TotalCount > 0 ? TotalDurationMs / TotalCount : 0.0f;
	}

	float GetSuccessRate() const
	{
		return TotalCount > 0 ? (static_cast<float>(SuccessCount) / TotalCount) * 100.0f : 0.0f;
	}
};

/**
 * Performance thresholds for inventory operations.
 */
USTRUCT(BlueprintType)
struct FInventoryPerformanceThresholds
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	float WarningMs = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	float CriticalMs = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	int32 MaxWarningsPerSecond = 10;
};

/**
 * A performance alert triggered when an operation exceeds thresholds.
 */
USTRUCT(BlueprintType)
struct FInventoryPerformanceAlert
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	EInventoryOperationType OpType = EInventoryOperationType::IOT_Other;

	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	float DurationMs = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	FString Context;

	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	bool bIsCritical = false;

	double Timestamp = 0.0;
};

/**
 * Ring-buffer based operation tracker for inventory system debugging.
 * Tracks operation history, per-type statistics, and success/failure rates.
 */
USTRUCT(BlueprintType)
struct FInventoryOperationTracker
{
	GENERATED_BODY()

private:
	UPROPERTY()
	TArray<FInventoryOperationRecord> History;

	UPROPERTY()
	int32 MaxHistorySize = 256;

	UPROPERTY()
	int32 CurrentIndex = 0;

	UPROPERTY()
	int32 RecordedCount = 0;

	UPROPERTY()
	TMap<EInventoryOperationType, FOperationTypeStats> TypeStats;

	UPROPERTY()
	int32 TotalOperations = 0;

	UPROPERTY()
	int32 SuccessfulOperations = 0;

	UPROPERTY()
	int32 FailedOperations = 0;

	UPROPERTY()
	bool bIsTracking = false;

	FInventoryPerformanceThresholds PerfThresholds;
	TArray<FInventoryPerformanceAlert> AlertHistory;
	int32 MaxAlertHistorySize = 64;
	int32 AlertCurrentIndex = 0;
	int32 AlertRecordedCount = 0;
	int32 WarningsThisSecond = 0;
	double LastWarningResetTime = 0.0;

public:
	void SetTracking(bool bEnabled)
	{
		bIsTracking = bEnabled;
		if (bEnabled && History.Num() == 0)
		{
			History.SetNum(MaxHistorySize);
		}
	}

	bool IsTracking() const { return bIsTracking; }

	void SetMaxHistorySize(int32 NewSize)
	{
		MaxHistorySize = FMath::Max(16, NewSize);
		History.SetNum(MaxHistorySize);
		CurrentIndex = FMath::Min(CurrentIndex, MaxHistorySize - 1);
	}

	void RecordOperation(EInventoryOperationType Type, const FInventoryOperationResult& Result,
	                     float DurationMs, const FString& Context = TEXT(""))
	{
		if (!bIsTracking)
		{
			return;
		}

		if (History.Num() == 0)
		{
			History.SetNum(MaxHistorySize);
		}

		FInventoryOperationRecord& Record = History[CurrentIndex];
		Record.OperationType = Type;
		Record.bSuccess = Result.bSuccess;
		Record.Message = Result.Message;
		Record.DurationMs = DurationMs;
		Record.ContextInfo = Context;
		Record.Timestamp = FPlatformTime::Seconds();

		CurrentIndex = (CurrentIndex + 1) % MaxHistorySize;
		RecordedCount++;

		TotalOperations++;
		if (Result.bSuccess)
		{
			SuccessfulOperations++;
		}
		else
		{
			FailedOperations++;
		}

		FOperationTypeStats& Stats = TypeStats.FindOrAdd(Type);
		Stats.TotalCount++;
		if (Result.bSuccess)
		{
			Stats.SuccessCount++;
		}
		else
		{
			Stats.FailCount++;
		}
		Stats.TotalDurationMs += DurationMs;
		Stats.MinDurationMs = FMath::Min(Stats.MinDurationMs, DurationMs);
		Stats.MaxDurationMs = FMath::Max(Stats.MaxDurationMs, DurationMs);

		if (DurationMs >= PerfThresholds.WarningMs)
		{
			double Now = FPlatformTime::Seconds();

			if (Now - LastWarningResetTime >= 1.0)
			{
				WarningsThisSecond = 0;
				LastWarningResetTime = Now;
			}

			if (WarningsThisSecond < PerfThresholds.MaxWarningsPerSecond)
			{
				WarningsThisSecond++;

				bool bCritical = DurationMs >= PerfThresholds.CriticalMs;

				if (AlertHistory.Num() == 0)
				{
					AlertHistory.SetNum(MaxAlertHistorySize);
				}

				FInventoryPerformanceAlert& Alert = AlertHistory[AlertCurrentIndex];
				Alert.OpType = Type;
				Alert.DurationMs = DurationMs;
				Alert.Context = Context;
				Alert.bIsCritical = bCritical;
				Alert.Timestamp = Now;

				AlertCurrentIndex = (AlertCurrentIndex + 1) % MaxAlertHistorySize;
				AlertRecordedCount++;
			}
		}
	}

	TArray<FInventoryOperationRecord> GetRecentOperations(int32 Count = 20) const
	{
		TArray<FInventoryOperationRecord> Result;
		const int32 ActualCount = FMath::Min(Count, FMath::Min(RecordedCount, MaxHistorySize));

		for (int32 i = 0; i < ActualCount; ++i)
		{
			int32 Index = (CurrentIndex - 1 - i + MaxHistorySize) % MaxHistorySize;
			if (Index >= 0 && Index < History.Num())
			{
				Result.Add(History[Index]);
			}
		}

		return Result;
	}

	TArray<FInventoryOperationRecord> GetFailedOperations(int32 Count = 20) const
	{
		TArray<FInventoryOperationRecord> Result;
		const int32 SearchCount = FMath::Min(RecordedCount, MaxHistorySize);

		for (int32 i = 0; i < SearchCount && Result.Num() < Count; ++i)
		{
			int32 Index = (CurrentIndex - 1 - i + MaxHistorySize) % MaxHistorySize;
			if (Index >= 0 && Index < History.Num() && !History[Index].bSuccess)
			{
				Result.Add(History[Index]);
			}
		}

		return Result;
	}

	float GetSuccessRate() const
	{
		return TotalOperations > 0
			       ? (static_cast<float>(SuccessfulOperations) / TotalOperations) * 100.0f
			       : 0.0f;
	}

	float GetSuccessRateForType(EInventoryOperationType Type) const
	{
		const FOperationTypeStats* Stats = TypeStats.Find(Type);
		return Stats ? Stats->GetSuccessRate() : 0.0f;
	}

	float GetAverageDuration(EInventoryOperationType Type) const
	{
		const FOperationTypeStats* Stats = TypeStats.Find(Type);
		return Stats ? Stats->GetAverageDurationMs() : 0.0f;
	}

	const FOperationTypeStats* GetStatsForType(EInventoryOperationType Type) const
	{
		return TypeStats.Find(Type);
	}

	const TMap<EInventoryOperationType, FOperationTypeStats>& GetAllTypeStats() const
	{
		return TypeStats;
	}

	int32 GetTotalOperations() const { return TotalOperations; }
	int32 GetSuccessfulOperations() const { return SuccessfulOperations; }
	int32 GetFailedOperations() const { return FailedOperations; }

	void SetPerformanceThresholds(const FInventoryPerformanceThresholds& NewThresholds)
	{
		PerfThresholds = NewThresholds;
	}

	const FInventoryPerformanceThresholds& GetPerformanceThresholds() const
	{
		return PerfThresholds;
	}

	TArray<FInventoryPerformanceAlert> GetRecentAlerts(int32 Count = 20) const
	{
		TArray<FInventoryPerformanceAlert> Result;
		const int32 ActualCount = FMath::Min(Count, FMath::Min(AlertRecordedCount, MaxAlertHistorySize));

		for (int32 i = 0; i < ActualCount; ++i)
		{
			int32 Index = (AlertCurrentIndex - 1 - i + MaxAlertHistorySize) % MaxAlertHistorySize;
			if (Index >= 0 && Index < AlertHistory.Num())
			{
				Result.Add(AlertHistory[Index]);
			}
		}

		return Result;
	}

	void Reset()
	{
		History.Empty();
		History.SetNum(MaxHistorySize);
		CurrentIndex = 0;
		RecordedCount = 0;
		TypeStats.Empty();
		TotalOperations = 0;
		SuccessfulOperations = 0;
		FailedOperations = 0;
		AlertHistory.Empty();
		AlertCurrentIndex = 0;
		AlertRecordedCount = 0;
		WarningsThisSecond = 0;
	}

	FString GetSummaryString() const
	{
		FString Summary = FString::Printf(
			TEXT("=== Operation Tracker Summary ===\n")
			TEXT("Total: %d | Success: %d | Failed: %d | Rate: %.1f%%\n"),
			TotalOperations, SuccessfulOperations, FailedOperations, GetSuccessRate());

		for (const auto& Pair : TypeStats)
		{
			const FOperationTypeStats& Stats = Pair.Value;
			Summary += FString::Printf(
				TEXT("  [%s] Count: %d | Success: %.1f%% | Avg: %.3fms | Min: %.3fms | Max: %.3fms\n"),
				*UEnum::GetValueAsString(Pair.Key),
				Stats.TotalCount,
				Stats.GetSuccessRate(),
				Stats.GetAverageDurationMs(),
				Stats.MinDurationMs == TNumericLimits<float>::Max() ? 0.0f : Stats.MinDurationMs,
				Stats.MaxDurationMs);
		}

		return Summary;
	}
};
