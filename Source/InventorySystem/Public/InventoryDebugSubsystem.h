#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Items/ItemBase.h"
#include "InventoryComponent.h"
#include "InventoryOperationTracker.h"
#include "InventoryDebugSubsystem.generated.h"

UENUM(BlueprintType)
enum class EInventoryDebugMode : uint8
{
	IDM_None            UMETA(DisplayName = "None"),
	IDM_Basic           UMETA(DisplayName = "Basic Info"),
	IDM_Detailed        UMETA(DisplayName = "Detailed Info"),
	IDM_Performance     UMETA(DisplayName = "Performance Stats"),
	IDM_Network         UMETA(DisplayName = "Network Info")
};

USTRUCT(BlueprintType)
struct FInventoryDebugStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	int32 TotalItems = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	int32 TotalSlots = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	int32 UsedSlots = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	int32 EmptySlots = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	float OccupancyPercentage = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	int32 UniqueItemTypes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	int32 StackableItems = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	float AverageStackSize = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	int32 MemoryUsageBytes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	int32 TotalGroups = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	TArray<FString> GroupBreakdowns;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	int32 InstalledModuleCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	int32 PooledItemsAvailable = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	int32 PooledItemsActive = 0;
};

/**
 * Game instance subsystem providing debugging, visualization, and cheat commands for the inventory system.
 * Only fully active in non-shipping builds; console commands and frame tracking are stripped in shipping.
 */
UCLASS()
class INVENTORYSYSTEM_API UInventoryDebugSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

protected:
	UPROPERTY()
	EInventoryDebugMode DebugMode = EInventoryDebugMode::IDM_None;

	UPROPERTY()
	bool bShowDebugOverlay = false;

	UPROPERTY()
	TArray<TObjectPtr<UInventoryComponent>> TrackedInventories;

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug")
	void SetDebugOverlay(bool bEnabled, EInventoryDebugMode Mode = EInventoryDebugMode::IDM_Basic);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug")
	void DrawInventoryDebug(UInventoryComponent* Inventory, UCanvas* Canvas, float& YPos);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug")
	FInventoryDebugStats GetInventoryStats(UInventoryComponent* Inventory);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug")
	void LogInventoryContents(UInventoryComponent* Inventory);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug")
	void DrawInventoryVisualization(UInventoryComponent* Inventory, const FVector& Location);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Spawner")
	UItemBase* SpawnItemByClass(TSubclassOf<UItemBase> ItemClass, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Spawner")
	UItemBase* SpawnRandomItem();

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Spawner")
	bool GiveItemToPlayer(TSubclassOf<UItemBase> ItemClass, int32 Quantity = 1, int32 PlayerIndex = 0);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Spawner")
	void FillInventoryRandom(UInventoryComponent* Inventory, int32 Count);

	UFUNCTION(Exec, Category = "Inventory|Debug|Cheats")
	void ClearPlayerInventory(int32 PlayerIndex = 0);

	UFUNCTION(Exec, Category = "Inventory|Debug|Cheats")
	void GiveAllItems(int32 PlayerIndex = 0);

	UFUNCTION(Exec, Category = "Inventory|Debug|Cheats")
	void SetItemStack(int32 GroupIndex, int32 SlotIndex, int32 StackSize);

	UFUNCTION(Exec, Category = "Inventory|Debug|Cheats")
	void DuplicateItem(int32 GroupIndex, int32 SlotIndex);

	UFUNCTION(Exec, Category = "Inventory|Debug|Cheats")
	void ToggleInfiniteStacks();

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Profiling")
	void StartProfiling();

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Profiling")
	void StopProfiling();

	/** Runs AddItem in a tight loop and returns average duration in milliseconds. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Profiling")
	float BenchmarkAddItem(UInventoryComponent* Inventory, int32 Iterations = 1000);

	/** Runs item searches in a tight loop and returns average duration in milliseconds. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Profiling")
	float BenchmarkSearch(UInventoryComponent* Inventory, int32 Iterations = 100);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Database")
	TArray<TSubclassOf<UItemBase>> GetAllItemClasses();

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Database")
	FString GetItemInfoString(UItemBase* Item);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Database")
	TArray<TSubclassOf<UItemBase>> SearchItemDatabase(const FString& SearchTerm);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Validation")
	bool ValidateInventory(UInventoryComponent* Inventory, TArray<FString>& OutErrors);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Testing")
	void RunAutomatedTests(UInventoryComponent* Inventory);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Testing")
	void TestNetworkReplication(UInventoryComponent* Inventory);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Tracking")
	void RecordOperation(EInventoryOperationType Type, const FInventoryOperationResult& Result,
	                     float DurationMs, const FString& Context = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Tracking")
	void SetOperationTracking(bool bEnabled);

	FInventoryOperationTracker& GetOperationTracker() { return OperationTracker; }

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Tracking")
	TArray<FInventoryOperationRecord> GetRecentOperations(int32 Count = 20);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Tracking")
	TArray<FInventoryOperationRecord> GetFailedOperations(int32 Count = 20);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Tracking")
	FString GetOperationSummary();

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Performance")
	void SetPerformanceThresholds(float WarningMs, float CriticalMs);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Performance")
	TArray<FInventoryPerformanceAlert> GetRecentPerformanceAlerts(int32 Count = 20);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Performance")
	void SetFrameTracking(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Performance")
	float GetCurrentFrameCostMs() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Performance")
	float GetAverageFrameCostMs() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug|Performance")
	float GetPeakFrameCostMs() const;

	bool IsInfiniteStacksActive() const { return bInfiniteStacks; }

protected:
	UInventoryComponent* GetPlayerInventory(int32 PlayerIndex);
	void RegisterItemClass(TSubclassOf<UItemBase> ItemClass);

	UPROPERTY()
	TArray<TSubclassOf<UItemBase>> RegisteredItemClasses;

	UPROPERTY()
	FInventoryOperationTracker OperationTracker;

	bool bIsProfiling = false;
	double ProfilingStartTime = 0.0;
	int32 ProfiledOperations = 0;

	bool bInfiniteStacks = false;

	TMap<UItemBase*, int32> OriginalMaxStackSizes;

#if !UE_BUILD_SHIPPING
	bool bFrameTrackingEnabled = false;
	float CurrentFrameCostMs = 0.0f;
	int32 CurrentFrameOpCount = 0;
	TArray<float> FrameCostHistory;
	int32 FrameCostHistoryIndex = 0;
	int32 FrameCostHistoryCount = 0;
	static constexpr int32 FrameCostHistorySize = 300;
	float PeakFrameCostMs = 0.0f;
	FDelegateHandle TickDelegateHandle;

	void OnWorldTickStart(UWorld* World, ELevelTick TickType, float DeltaSeconds);
#endif
};

/**
 * Records an inventory operation into the debug subsystem.
 * No-ops in shipping builds.
 */
#if !UE_BUILD_SHIPPING
inline void TrackInventoryOperation(UWorld* World, EInventoryOperationType Type,
                                     const FInventoryOperationResult& Result,
                                     float DurationMs, const FString& Context = TEXT(""))
{
	if (!World) return;
	if (UGameInstance* GI = World->GetGameInstance())
	{
		if (UInventoryDebugSubsystem* DS = GI->GetSubsystem<UInventoryDebugSubsystem>())
		{
			DS->RecordOperation(Type, Result, DurationMs, Context);
		}
	}
}

/**
 * RAII timer that records an inventory operation's duration on destruction.
 * Bind a result pointer via SetResult before the scope ends for accurate success tracking.
 */
struct FScopedInventoryOperationTimer
{
	UWorld* World;
	EInventoryOperationType OpType;
	FString Context;
	double StartTime;
	const FInventoryOperationResult* ResultPtr = nullptr;

	FScopedInventoryOperationTimer(UWorld* InWorld, EInventoryOperationType InOpType, const FString& InContext = TEXT(""))
		: World(InWorld), OpType(InOpType), Context(InContext), StartTime(FPlatformTime::Seconds())
	{
	}

	void SetResult(const FInventoryOperationResult* InResult) { ResultPtr = InResult; }

	~FScopedInventoryOperationTimer()
	{
		float DurationMs = static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0);
		if (ResultPtr)
		{
			TrackInventoryOperation(World, OpType, *ResultPtr, DurationMs, Context);
		}
		else
		{
			FInventoryOperationResult DefaultResult = FInventoryOperationResult::Ok();
			TrackInventoryOperation(World, OpType, DefaultResult, DurationMs, Context);
		}
	}
};

#define SCOPED_INVENTORY_TRACK(World, OpType, Context) \
	FScopedInventoryOperationTimer _ScopedInvTracker(World, OpType, Context)

#define SCOPED_INVENTORY_TRACK_RESULT(World, OpType, Context, ResultPtr) \
	FScopedInventoryOperationTimer _ScopedInvTracker(World, OpType, Context); \
	_ScopedInvTracker.SetResult(ResultPtr)

#else
inline void TrackInventoryOperation(UWorld*, EInventoryOperationType,
                                     const FInventoryOperationResult&,
                                     float, const FString& = TEXT("")) {}
#define SCOPED_INVENTORY_TRACK(World, OpType, Context)
#define SCOPED_INVENTORY_TRACK_RESULT(World, OpType, Context, ResultPtr)
#endif
