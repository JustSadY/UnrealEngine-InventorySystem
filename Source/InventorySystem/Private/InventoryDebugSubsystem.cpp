#include "InventoryDebugSubsystem.h"
#include "InventorySystem.h"
#include "Engine/Canvas.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "PoolSystem/ItemPoolSubsystem.h"
#include "Modules/InventoryModuleBase.h"

void UInventoryDebugSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogInventory, Log, TEXT("InventoryDebugSubsystem initialized"));
}

void UInventoryDebugSubsystem::Deinitialize()
{
#if !UE_BUILD_SHIPPING
	if (TickDelegateHandle.IsValid())
	{
		FWorldDelegates::OnWorldTickStart.Remove(TickDelegateHandle);
		TickDelegateHandle.Reset();
	}
#endif

	TrackedInventories.Empty();
	Super::Deinitialize();
}

void UInventoryDebugSubsystem::SetDebugOverlay(bool bEnabled, EInventoryDebugMode Mode)
{
	bShowDebugOverlay = bEnabled;
	DebugMode = Mode;

	if (bEnabled)
	{
		UE_LOG(LogInventory, Log, TEXT("Inventory debug overlay enabled - Mode: %d"), (int32)Mode);
	}
	else
	{
		UE_LOG(LogInventory, Log, TEXT("Inventory debug overlay disabled"));
	}
}

void UInventoryDebugSubsystem::DrawInventoryDebug(UInventoryComponent* Inventory, UCanvas* Canvas, float& YPos)
{
	if (!Inventory || !Canvas || !bShowDebugOverlay)
	{
		return;
	}

	FInventoryDebugStats Stats = GetInventoryStats(Inventory);
	float XPos = 10.0f;
	float LineHeight = 20.0f;

	FCanvasTileItem BackgroundTile(
		FVector2D(XPos - 5, YPos - 5),
		FVector2D(400, 200),
		FLinearColor(0.0f, 0.0f, 0.0f, 0.5f)
	);
	Canvas->DrawItem(BackgroundTile);

	FCanvasTextItem TitleText(
		FVector2D(XPos, YPos),
		FText::FromString(TEXT("INVENTORY DEBUG")),
		GEngine->GetLargeFont(),
		FLinearColor::Yellow
	);
	Canvas->DrawItem(TitleText);
	YPos += LineHeight * 1.5f;

	FString StatsText = FString::Printf(TEXT("Total Items: %d / %d slots"), Stats.TotalItems, Stats.TotalSlots);
	FCanvasTextItem Text1(FVector2D(XPos, YPos), FText::FromString(StatsText), GEngine->GetMediumFont(),
	                      FLinearColor::White);
	Canvas->DrawItem(Text1);
	YPos += LineHeight;

	StatsText = FString::Printf(TEXT("Occupancy: %.1f%% (%d used, %d empty)"),
	                            Stats.OccupancyPercentage, Stats.UsedSlots, Stats.EmptySlots);
	FCanvasTextItem Text2(FVector2D(XPos, YPos), FText::FromString(StatsText), GEngine->GetMediumFont(),
	                      FLinearColor::White);
	Canvas->DrawItem(Text2);
	YPos += LineHeight;

	if (DebugMode >= EInventoryDebugMode::IDM_Detailed)
	{
		StatsText = FString::Printf(TEXT("Unique Types: %d"), Stats.UniqueItemTypes);
		FCanvasTextItem Text3(FVector2D(XPos, YPos), FText::FromString(StatsText), GEngine->GetMediumFont(),
		                      FLinearColor(0.0f, 1.0f, 1.0f));
		Canvas->DrawItem(Text3);
		YPos += LineHeight;

		StatsText = FString::Printf(TEXT("Stackable: %d (Avg: %.1f)"),
		                            Stats.StackableItems, Stats.AverageStackSize);
		FCanvasTextItem Text4(FVector2D(XPos, YPos), FText::FromString(StatsText), GEngine->GetMediumFont(),
		                      FLinearColor(0.0f, 1.0f, 1.0f));
		Canvas->DrawItem(Text4);
		YPos += LineHeight;
	}

	if (DebugMode >= EInventoryDebugMode::IDM_Performance)
	{
#if !UE_BUILD_SHIPPING
		if (bFrameTrackingEnabled)
		{
			StatsText = FString::Printf(TEXT("Frame Cost: %.3fms | Avg: %.3fms | Peak: %.3fms"),
			                            CurrentFrameCostMs, GetAverageFrameCostMs(), PeakFrameCostMs);
			FLinearColor FrameColor = CurrentFrameCostMs > 1.0f
				                          ? FLinearColor::Red
				                          : (CurrentFrameCostMs > 0.5f ? FLinearColor::Yellow : FLinearColor::Green);
			FCanvasTextItem TextFrame(FVector2D(XPos, YPos), FText::FromString(StatsText), GEngine->GetMediumFont(),
			                          FrameColor);
			Canvas->DrawItem(TextFrame);
			YPos += LineHeight;
		}
#endif

		StatsText = FString::Printf(TEXT("Memory: %.2f KB"), Stats.MemoryUsageBytes / 1024.0f);
		FCanvasTextItem Text5(FVector2D(XPos, YPos), FText::FromString(StatsText), GEngine->GetMediumFont(),
		                      FLinearColor::Green);
		Canvas->DrawItem(Text5);
		YPos += LineHeight;

		StatsText = FString::Printf(TEXT("Modules: %d | Pool: %d avail / %d active"),
		                            Stats.InstalledModuleCount, Stats.PooledItemsAvailable, Stats.PooledItemsActive);
		FCanvasTextItem TextModules(FVector2D(XPos, YPos), FText::FromString(StatsText), GEngine->GetMediumFont(),
		                            FLinearColor::Green);
		Canvas->DrawItem(TextModules);
		YPos += LineHeight;

		if (OperationTracker.IsTracking())
		{
			StatsText = FString::Printf(TEXT("Ops: %d | Success: %.1f%%"),
			                            OperationTracker.GetTotalOperations(), OperationTracker.GetSuccessRate());
			FCanvasTextItem TextTracker(FVector2D(XPos, YPos), FText::FromString(StatsText), GEngine->GetMediumFont(),
			                            FLinearColor(1.0f, 0.5f, 0.0f));
			Canvas->DrawItem(TextTracker);
			YPos += LineHeight;

			TArray<FInventoryOperationRecord> FailedOps = OperationTracker.GetFailedOperations(3);
			for (const FInventoryOperationRecord& FailedOp : FailedOps)
			{
				StatsText = FString::Printf(TEXT("  FAIL: %s - %s"),
				                            *UEnum::GetValueAsString(FailedOp.OperationType), *FailedOp.Message);
				FCanvasTextItem TextFail(FVector2D(XPos, YPos), FText::FromString(StatsText), GEngine->GetMediumFont(),
				                         FLinearColor::Red);
				Canvas->DrawItem(TextFail);
				YPos += LineHeight;
			}

			TArray<FInventoryPerformanceAlert> Alerts = OperationTracker.GetRecentAlerts(3);
			for (const FInventoryPerformanceAlert& Alert : Alerts)
			{
				FLinearColor AlertColor = Alert.bIsCritical
					                          ? FLinearColor(1.0f, 0.0f, 0.0f)
					                          : FLinearColor(1.0f, 1.0f, 0.0f);
				StatsText = FString::Printf(TEXT("  %s: %s %.3fms - %s"),
				                            Alert.bIsCritical ? TEXT("CRIT") : TEXT("WARN"),
				                            *UEnum::GetValueAsString(Alert.OpType), Alert.DurationMs, *Alert.Context);
				FCanvasTextItem TextAlert(FVector2D(XPos, YPos), FText::FromString(StatsText), GEngine->GetMediumFont(),
				                          AlertColor);
				Canvas->DrawItem(TextAlert);
				YPos += LineHeight;
			}
		}
	}
}

FInventoryDebugStats UInventoryDebugSubsystem::GetInventoryStats(UInventoryComponent* Inventory)
{
	FInventoryDebugStats Stats;

	if (!Inventory)
	{
		return Stats;
	}

	TSet<FString> UniqueTypes;
	int32 TotalStackSize = 0;
	int32 StackableCount = 0;

	const TArray<FInventorySlots>& Groups = Inventory->GetInventorySlotsGroup().GetInventoryGroups();
	Stats.TotalGroups = Groups.Num();

	for (int32 GroupIdx = 0; GroupIdx < Groups.Num(); ++GroupIdx)
	{
		const FInventorySlots& Group = Groups[GroupIdx];
		int32 GroupUsed = Group.GetOccupiedSlotCount();
		int32 GroupTotal = Group.GetMaxSlotSize();
		Stats.TotalSlots += GroupTotal;

		FString TypeNames;
		for (const auto& Pair : Group.GetTypeIDMap())
		{
			if (!TypeNames.IsEmpty()) TypeNames += TEXT(", ");
			TypeNames += FString::Printf(TEXT("%d:%s"), Pair.Key, *Pair.Value);
		}
		Stats.GroupBreakdowns.Add(FString::Printf(TEXT("Group %d [%s]: %d/%d slots"),
		                                          GroupIdx, *TypeNames, GroupUsed, GroupTotal));

		for (const FInventorySlot& Slot : Group.GetSlots())
		{
			if (!Slot.IsEmpty())
			{
				Stats.UsedSlots++;
				Stats.TotalItems += Slot.GetCurrentStackSize();

				UItemBase* Item = Slot.GetItem();
				if (Item)
				{
					UniqueTypes.Add(Item->GetItemDefinition().GetItemID());

					if (Item->IsStackable())
					{
						StackableCount++;
						TotalStackSize += Slot.GetCurrentStackSize();
					}
				}
			}
		}
	}

	Stats.EmptySlots = Stats.TotalSlots - Stats.UsedSlots;
	Stats.OccupancyPercentage = Stats.TotalSlots > 0 ? (Stats.UsedSlots * 100.0f) / Stats.TotalSlots : 0.0f;
	Stats.UniqueItemTypes = UniqueTypes.Num();
	Stats.StackableItems = StackableCount;
	Stats.AverageStackSize = StackableCount > 0 ? (float)TotalStackSize / StackableCount : 0.0f;

	Stats.MemoryUsageBytes = Stats.TotalSlots * static_cast<int32>(sizeof(FInventorySlot))
		+ Stats.UsedSlots * 512;

	Stats.InstalledModuleCount = 0;
	TArray<UItemBase*> AllItems = Inventory->GetAllItems();
	for (UItemBase* Item : AllItems)
	{
		if (Item)
		{
			Stats.InstalledModuleCount += Item->GetAllModules().Num();
		}
	}

	Stats.PooledItemsAvailable = 0;
	Stats.PooledItemsActive = 0;
	if (UWorld* World = Inventory->GetWorld())
	{
		if (UItemPoolSubsystem* PoolSys = World->GetSubsystem<UItemPoolSubsystem>())
		{
			for (UItemBase* Item : AllItems)
			{
				if (Item)
				{
					int32 Available = 0, Active = 0, Total = 0;
					PoolSys->GetPoolStats(Item->GetClass(), Available, Active, Total);
					Stats.PooledItemsAvailable += Available;
					Stats.PooledItemsActive += Active;
				}
			}
		}
	}

	return Stats;
}

void UInventoryDebugSubsystem::LogInventoryContents(UInventoryComponent* Inventory)
{
	if (!Inventory)
	{
		UE_LOG(LogInventory, Error, TEXT("LogInventoryContents: Invalid inventory"));
		return;
	}

	UE_LOG(LogInventory, Log, TEXT("=== INVENTORY CONTENTS ==="));

	const TArray<FInventorySlots>& Groups = Inventory->GetInventorySlotsGroup().GetInventoryGroups();

	for (int32 GroupIdx = 0; GroupIdx < Groups.Num(); ++GroupIdx)
	{
		const FInventorySlots& Group = Groups[GroupIdx];
		UE_LOG(LogInventory, Log, TEXT("Group %d: %d/%d slots used"),
		       GroupIdx, Group.GetOccupiedSlotCount(), Group.GetMaxSlotSize());

		for (int32 SlotIdx = 0; SlotIdx < Group.GetSlots().Num(); ++SlotIdx)
		{
			const FInventorySlot& Slot = Group.GetSlots()[SlotIdx];
			if (!Slot.IsEmpty())
			{
				UItemBase* Item = Slot.GetItem();
				UE_LOG(LogInventory, Log, TEXT("  [%d] %s x%d"),
				       SlotIdx,
				       *Item->GetItemDefinition().GetItemName().ToString(),
				       Slot.GetCurrentStackSize());
			}
		}
	}

	UE_LOG(LogInventory, Log, TEXT("=== END INVENTORY ==="));
}

void UInventoryDebugSubsystem::DrawInventoryVisualization(UInventoryComponent* Inventory, const FVector& Location)
{
	if (!Inventory || !Inventory->GetWorld())
	{
		return;
	}

	const TArray<FInventorySlots>& Groups = Inventory->GetInventorySlotsGroup().GetInventoryGroups();
	FVector CurrentPos = Location;
	float SlotSpacing = 100.0f;

	for (const FInventorySlots& Group : Groups)
	{
		for (const FInventorySlot& Slot : Group.GetSlots())
		{
			FColor Color = Slot.IsEmpty() ? FColor::Red : FColor::Green;
			DrawDebugBox(Inventory->GetWorld(), CurrentPos, FVector(40.0f), Color, false, 0.1f, 0, 2.0f);

			if (!Slot.IsEmpty())
			{
				FString Text = FString::Printf(TEXT("%d"), Slot.GetCurrentStackSize());
				DrawDebugString(Inventory->GetWorld(), CurrentPos + FVector(0, 0, 50), Text, nullptr, FColor::White,
				                0.1f);
			}

			CurrentPos.Y += SlotSpacing;
		}

		CurrentPos.X += SlotSpacing;
		CurrentPos.Y = Location.Y;
	}
}

UItemBase* UInventoryDebugSubsystem::SpawnItemByClass(TSubclassOf<UItemBase> ItemClass, int32 Quantity)
{
	if (!ItemClass)
	{
		UE_LOG(LogInventory, Error, TEXT("SpawnItemByClass: Invalid item class"));
		return nullptr;
	}

	UItemBase* NewItem = NewObject<UItemBase>(this, ItemClass);
	if (NewItem)
	{
		NewItem->InitializeItem();
		NewItem->SetCurrentStackSize(Quantity);
		UE_LOG(LogInventory, Log, TEXT("Spawned item: %s x%d"), *NewItem->GetItemDefinition().GetItemName().ToString(),
		       Quantity);
	}

	return NewItem;
}

UItemBase* UInventoryDebugSubsystem::SpawnRandomItem()
{
	if (RegisteredItemClasses.Num() == 0)
	{
		UE_LOG(LogInventory, Warning, TEXT("No registered item classes for random spawn"));
		return nullptr;
	}

	int32 RandomIndex = FMath::RandRange(0, RegisteredItemClasses.Num() - 1);
	return SpawnItemByClass(RegisteredItemClasses[RandomIndex], FMath::RandRange(1, 10));
}

bool UInventoryDebugSubsystem::GiveItemToPlayer(TSubclassOf<UItemBase> ItemClass, int32 Quantity, int32 PlayerIndex)
{
	UInventoryComponent* PlayerInv = GetPlayerInventory(PlayerIndex);
	if (!PlayerInv)
	{
		UE_LOG(LogInventory, Error, TEXT("GiveItemToPlayer: Could not find player inventory"));
		return false;
	}

	return PlayerInv->AddItemByClass(ItemClass, Quantity).bSuccess;
}

void UInventoryDebugSubsystem::FillInventoryRandom(UInventoryComponent* Inventory, int32 Count)
{
	if (!Inventory)
	{
		return;
	}

	for (int32 i = 0; i < Count; ++i)
	{
		UItemBase* RandomItem = SpawnRandomItem();
		if (RandomItem)
		{
			Inventory->AddItem(RandomItem);
		}
	}

	UE_LOG(LogInventory, Log, TEXT("Filled inventory with %d random items"), Count);
}

void UInventoryDebugSubsystem::ClearPlayerInventory(int32 PlayerIndex)
{
	UInventoryComponent* PlayerInv = GetPlayerInventory(PlayerIndex);
	if (PlayerInv)
	{
		PlayerInv->ClearInventory();
		UE_LOG(LogInventory, Log, TEXT("Cleared player %d inventory"), PlayerIndex);
	}
}

void UInventoryDebugSubsystem::GiveAllItems(int32 PlayerIndex)
{
	UInventoryComponent* PlayerInv = GetPlayerInventory(PlayerIndex);
	if (!PlayerInv)
	{
		return;
	}

	for (TSubclassOf<UItemBase> ItemClass : RegisteredItemClasses)
	{
		PlayerInv->AddItemByClass(ItemClass, 1);
	}

	UE_LOG(LogInventory, Log, TEXT("Gave all items to player %d"), PlayerIndex);
}

void UInventoryDebugSubsystem::SetItemStack(int32 GroupIndex, int32 SlotIndex, int32 StackSize)
{
	UInventoryComponent* PlayerInv = GetPlayerInventory(0);
	if (!PlayerInv)
	{
		return;
	}

	UItemBase* Item = PlayerInv->GetItemAtIndex(GroupIndex, SlotIndex);
	if (Item)
	{
		Item->SetCurrentStackSize(StackSize);
		UE_LOG(LogInventory, Log, TEXT("Set stack size to %d"), StackSize);
	}
}

void UInventoryDebugSubsystem::DuplicateItem(int32 GroupIndex, int32 SlotIndex)
{
	UInventoryComponent* PlayerInv = GetPlayerInventory(0);
	if (!PlayerInv)
	{
		return;
	}

	UItemBase* Item = PlayerInv->GetItemAtIndex(GroupIndex, SlotIndex);
	if (Item)
	{
		PlayerInv->AddItemByClass(Item->GetClass(), Item->GetCurrentStackSize());
		UE_LOG(LogInventory, Log, TEXT("Duplicated item"));
	}
}

void UInventoryDebugSubsystem::ToggleInfiniteStacks()
{
	bInfiniteStacks = !bInfiniteStacks;

	UInventoryComponent* PlayerInv = GetPlayerInventory(0);
	if (!PlayerInv)
	{
		UE_LOG(LogInventory, Log, TEXT("Infinite stacks %s (no active inventory to modify)"),
		       bInfiniteStacks ? TEXT("ENABLED") : TEXT("DISABLED"));
		return;
	}

	TArray<UItemBase*> AllItems = PlayerInv->GetAllItems();

	if (bInfiniteStacks)
	{
		OriginalMaxStackSizes.Empty();
		for (UItemBase* Item : AllItems)
		{
			if (Item)
			{
				OriginalMaxStackSizes.Add(Item, Item->GetMaxStackSize());
			}
		}
		UE_LOG(LogInventory, Log,
		       TEXT(
			       "Infinite stacks ENABLED - saved %d original stack sizes. Flag active for IsInfiniteStacksActive() checks."
		       ),
		       OriginalMaxStackSizes.Num());
	}
	else
	{
		OriginalMaxStackSizes.Empty();
		UE_LOG(LogInventory, Log, TEXT("Infinite stacks DISABLED - original limits restored."));
	}
}

void UInventoryDebugSubsystem::StartProfiling()
{
	bIsProfiling = true;
	ProfilingStartTime = FPlatformTime::Seconds();
	ProfiledOperations = 0;
	UE_LOG(LogInventory, Log, TEXT("Started inventory profiling"));
}

void UInventoryDebugSubsystem::StopProfiling()
{
	if (!bIsProfiling)
	{
		return;
	}

	double ElapsedTime = FPlatformTime::Seconds() - ProfilingStartTime;
	float OperationsPerSecond = (ElapsedTime > SMALL_NUMBER) ? ProfiledOperations / (float)ElapsedTime : 0.0f;

	UE_LOG(LogInventory, Log, TEXT("=== PROFILING RESULTS ==="));
	UE_LOG(LogInventory, Log, TEXT("Duration: %.3f seconds"), ElapsedTime);
	UE_LOG(LogInventory, Log, TEXT("Operations: %d"), ProfiledOperations);
	UE_LOG(LogInventory, Log, TEXT("Ops/sec: %.2f"), OperationsPerSecond);

	bIsProfiling = false;
}

float UInventoryDebugSubsystem::BenchmarkAddItem(UInventoryComponent* Inventory, int32 Iterations)
{
	if (!Inventory)
	{
		return 0.0f;
	}

	TSubclassOf<UItemBase> TestClass = RegisteredItemClasses.Num() > 0 ? RegisteredItemClasses[0] : nullptr;
	if (!TestClass)
	{
		return 0.0f;
	}

	double StartTime = FPlatformTime::Seconds();

	for (int32 i = 0; i < Iterations; ++i)
	{
		UItemBase* Item = NewObject<UItemBase>(Inventory, TestClass);
		Item->InitializeItem();
		Inventory->AddItem(Item);
	}

	double ElapsedTime = FPlatformTime::Seconds() - StartTime;
	float AvgTime = (ElapsedTime / Iterations) * 1000.0f;

	UE_LOG(LogInventory, Log, TEXT("AddItem benchmark: %.3f ms avg (%d iterations)"), AvgTime, Iterations);

	return AvgTime;
}

float UInventoryDebugSubsystem::BenchmarkSearch(UInventoryComponent* Inventory, int32 Iterations)
{
	if (!Inventory)
	{
		return 0.0f;
	}

	TArray<UItemBase*> AllItems = Inventory->GetAllItems();
	if (AllItems.Num() == 0)
	{
		return 0.0f;
	}

	UItemBase* SearchItem = AllItems[0];
	double StartTime = FPlatformTime::Seconds();

	for (int32 i = 0; i < Iterations; ++i)
	{
		int32 TypeID, SlotIndex;
		Inventory->FindItemLocation(SearchItem, TypeID, SlotIndex);
	}

	double ElapsedTime = FPlatformTime::Seconds() - StartTime;
	float AvgTime = (ElapsedTime / Iterations) * 1000.0f;

	UE_LOG(LogInventory, Log, TEXT("Search benchmark: %.3f ms avg (%d iterations)"), AvgTime, Iterations);

	return AvgTime;
}

TArray<TSubclassOf<UItemBase>> UInventoryDebugSubsystem::GetAllItemClasses()
{
	return RegisteredItemClasses;
}

FString UInventoryDebugSubsystem::GetItemInfoString(UItemBase* Item)
{
	if (!Item)
	{
		return TEXT("Invalid Item");
	}

	return FString::Printf(TEXT("%s [%s] - Stack: %d/%d"),
	                       *Item->GetItemDefinition().GetItemName().ToString(),
	                       *Item->GetItemDefinition().GetItemID(),
	                       Item->GetCurrentStackSize(),
	                       Item->GetMaxStackSize());
}

TArray<TSubclassOf<UItemBase>> UInventoryDebugSubsystem::SearchItemDatabase(const FString& SearchTerm)
{
	TArray<TSubclassOf<UItemBase>> Results;

	for (TSubclassOf<UItemBase> ItemClass : RegisteredItemClasses)
	{
		if (ItemClass)
		{
			FString ClassName = ItemClass->GetName();
			if (ClassName.Contains(SearchTerm, ESearchCase::IgnoreCase))
			{
				Results.Add(ItemClass);
			}
		}
	}

	return Results;
}

bool UInventoryDebugSubsystem::ValidateInventory(UInventoryComponent* Inventory, TArray<FString>& OutErrors)
{
	OutErrors.Empty();

	if (!Inventory)
	{
		OutErrors.Add(TEXT("Inventory is null"));
		return false;
	}

	const TArray<FInventorySlots>& Groups = Inventory->GetInventorySlotsGroup().GetInventoryGroups();
	for (int32 i = 0; i < Groups.Num(); ++i)
	{
		const FInventorySlots& Group = Groups[i];
		for (int32 j = 0; j < Group.GetSlots().Num(); ++j)
		{
			const FInventorySlot& Slot = Group.GetSlots()[j];
			if (!Slot.IsEmpty() && !IsValid(Slot.GetItem()))
			{
				OutErrors.Add(FString::Printf(TEXT("Group %d, Slot %d: Invalid item reference"), i, j));
			}
		}
	}

	return OutErrors.Num() == 0;
}

void UInventoryDebugSubsystem::RunAutomatedTests(UInventoryComponent* Inventory)
{
	if (!Inventory)
	{
		UE_LOG(LogInventory, Error, TEXT("RunAutomatedTests: Invalid inventory"));
		return;
	}

	if (RegisteredItemClasses.Num() == 0)
	{
		UE_LOG(LogInventory, Error, TEXT("RunAutomatedTests: No registered item classes - cannot run tests"));
		return;
	}

	int32 Passed = 0;
	int32 Failed = 0;
	TSubclassOf<UItemBase> TestClass = RegisteredItemClasses[0];

	UE_LOG(LogInventory, Log, TEXT("=== Running Automated Inventory Tests ==="));
	UE_LOG(LogInventory, Log, TEXT("Test item class: %s"), *TestClass->GetName());

	int32 InitialItemCount = Inventory->GetAllItems().Num();

	{
		UE_LOG(LogInventory, Log, TEXT("Test 1: AddItem"));
		UItemBase* TestItem = NewObject<UItemBase>(Inventory, TestClass);
		TestItem->InitializeItem();

		FInventoryOperationResult Result = Inventory->AddItem(TestItem);
		if (Result.bSuccess)
		{
			int32 TypeID, SlotIndex;
			bool bFound = Inventory->FindItemLocation(TestItem, TypeID, SlotIndex);
			if (bFound)
			{
				UE_LOG(LogInventory, Log, TEXT("  PASS - Item added at Group %d, Slot %d"), TypeID, SlotIndex);
				Passed++;
			}
			else
			{
				UE_LOG(LogInventory, Error, TEXT("  FAIL - Item added but not found in inventory"));
				Failed++;
			}
		}
		else
		{
			UE_LOG(LogInventory, Error, TEXT("  FAIL - AddItem returned: %s"), *Result.Message);
			Failed++;
		}
	}

	{
		UE_LOG(LogInventory, Log, TEXT("Test 2: RemoveItem"));
		TArray<UItemBase*> Items = Inventory->GetAllItems();
		if (Items.Num() > 0)
		{
			UItemBase* ItemToRemove = Items.Last();
			FInventoryOperationResult Result = Inventory->RemoveItem(ItemToRemove);
			if (Result.bSuccess)
			{
				int32 TypeID, SlotIndex;
				bool bStillFound = Inventory->FindItemLocation(ItemToRemove, TypeID, SlotIndex);
				if (!bStillFound)
				{
					UE_LOG(LogInventory, Log, TEXT("  PASS - Item removed successfully"));
					Passed++;
				}
				else
				{
					UE_LOG(LogInventory, Error, TEXT("  FAIL - Item still found after removal"));
					Failed++;
				}
			}
			else
			{
				UE_LOG(LogInventory, Error, TEXT("  FAIL - RemoveItem returned: %s"), *Result.Message);
				Failed++;
			}
		}
		else
		{
			UE_LOG(LogInventory, Warning, TEXT("  SKIP - No items to remove"));
		}
	}

	{
		UE_LOG(LogInventory, Log, TEXT("Test 3: Stacking"));
		UItemBase* StackItem1 = NewObject<UItemBase>(Inventory, TestClass);
		StackItem1->InitializeItem();
		StackItem1->SetCurrentStackSize(1);

		FInventoryOperationResult AddResult = Inventory->AddItem(StackItem1);
		if (AddResult.bSuccess && StackItem1->IsStackable())
		{
			UItemBase* StackItem2 = NewObject<UItemBase>(Inventory, TestClass);
			StackItem2->InitializeItem();
			StackItem2->SetCurrentStackSize(1);

			FInventoryOperationResult StackResult = Inventory->TryStackItem(StackItem2);
			if (StackResult.bSuccess)
			{
				UE_LOG(LogInventory, Log, TEXT("  PASS - Stack operation succeeded"));
				Passed++;
			}
			else
			{
				UE_LOG(LogInventory, Error, TEXT("  FAIL - TryStackItem returned: %s"), *StackResult.Message);
				Failed++;
			}
		}
		else if (!StackItem1->IsStackable())
		{
			UE_LOG(LogInventory, Warning, TEXT("  SKIP - Test item is not stackable"));
			Inventory->RemoveItem(StackItem1);
		}
		else
		{
			UE_LOG(LogInventory, Error, TEXT("  FAIL - Could not add initial item for stacking: %s"),
			       *AddResult.Message);
			Failed++;
		}
	}

	{
		UE_LOG(LogInventory, Log, TEXT("Test 4: TransferItem"));
		const TArray<FInventorySlots>& Groups = Inventory->GetInventorySlotsGroup().GetInventoryGroups();
		if (Groups.Num() > 0 && Groups[0].GetMaxSlotSize() >= 2)
		{
			UItemBase* TransferItem = NewObject<UItemBase>(Inventory, TestClass);
			TransferItem->InitializeItem();
			Inventory->AddItem(TransferItem);

			int32 TypeID, SlotIndex;
			if (Inventory->FindItemLocation(TransferItem, TypeID, SlotIndex))
			{
				int32 TargetSlot = -1;
				const FInventorySlots* Group = Inventory->GetInventorySlotsGroup().GetGroupByID(TypeID);
				if (Group)
				{
					for (int32 i = 0; i < Group->GetSlots().Num(); ++i)
					{
						if (Group->GetSlots()[i].IsEmpty())
						{
							TargetSlot = i;
							break;
						}
					}
				}

				if (TargetSlot >= 0)
				{
					FInventoryOperationResult TransResult = Inventory->TransferItem(
						TypeID, SlotIndex, TypeID, TargetSlot);
					if (TransResult.bSuccess)
					{
						UE_LOG(LogInventory, Log, TEXT("  PASS - Transfer from slot %d to %d succeeded"), SlotIndex,
						       TargetSlot);
						Passed++;
					}
					else
					{
						UE_LOG(LogInventory, Error, TEXT("  FAIL - TransferItem returned: %s"), *TransResult.Message);
						Failed++;
					}
				}
				else
				{
					UE_LOG(LogInventory, Warning, TEXT("  SKIP - No empty slot available for transfer"));
				}
			}
			else
			{
				UE_LOG(LogInventory, Error, TEXT("  FAIL - Could not find item after adding"));
				Failed++;
			}
		}
		else
		{
			UE_LOG(LogInventory, Warning, TEXT("  SKIP - Need at least 2 slots in a group"));
		}
	}

	{
		UE_LOG(LogInventory, Log, TEXT("Test 5: Invalid Operations"));
		bool bAllInvalidsHandled = true;

		FInventoryOperationResult NullResult = Inventory->AddItem(nullptr);
		if (NullResult.bSuccess)
		{
			UE_LOG(LogInventory, Error, TEXT("  FAIL - AddItem(nullptr) should fail"));
			bAllInvalidsHandled = false;
		}

		FInventoryOperationResult BadIndexResult = Inventory->RemoveItemAt(-1, -1);
		if (BadIndexResult.bSuccess)
		{
			UE_LOG(LogInventory, Error, TEXT("  FAIL - RemoveItemAt(-1, -1) should fail"));
			bAllInvalidsHandled = false;
		}

		FInventoryOperationResult BadTransfer = Inventory->TransferItem(-1, -1, -1, -1);
		if (BadTransfer.bSuccess)
		{
			UE_LOG(LogInventory, Error, TEXT("  FAIL - TransferItem with invalid indices should fail"));
			bAllInvalidsHandled = false;
		}

		if (bAllInvalidsHandled)
		{
			UE_LOG(LogInventory, Log, TEXT("  PASS - All invalid operations properly rejected"));
			Passed++;
		}
		else
		{
			Failed++;
		}
	}

	{
		UE_LOG(LogInventory, Log, TEXT("Test 6: Inventory Full"));
		int32 EmptyCount = Inventory->GetEmptySlotCount();
		int32 FilledCount = 0;
		for (int32 i = 0; i < EmptyCount; ++i)
		{
			UItemBase* FillItem = NewObject<UItemBase>(Inventory, TestClass);
			FillItem->InitializeItem();
			FillItem->SetCurrentStackSize(FillItem->GetMaxStackSize());
			if (Inventory->AddItem(FillItem).bSuccess)
			{
				FilledCount++;
			}
		}

		UItemBase* OverflowItem = NewObject<UItemBase>(Inventory, TestClass);
		OverflowItem->InitializeItem();
		FInventoryOperationResult OverflowResult = Inventory->AddItem(OverflowItem);
		if (!OverflowResult.bSuccess)
		{
			UE_LOG(LogInventory, Log, TEXT("  PASS - Overflow correctly rejected after filling %d slots"), FilledCount);
			Passed++;
		}
		else
		{
			UE_LOG(LogInventory, Error, TEXT("  FAIL - Item added to supposedly full inventory"));
			Failed++;
		}
	}

	Inventory->ClearInventory();
	UE_LOG(LogInventory, Log, TEXT("=== Tests Complete: %d PASSED, %d FAILED ==="), Passed, Failed);
}

void UInventoryDebugSubsystem::TestNetworkReplication(UInventoryComponent* Inventory)
{
	if (!Inventory)
	{
		UE_LOG(LogInventory, Error, TEXT("TestNetworkReplication: Invalid inventory"));
		return;
	}

	int32 Issues = 0;
	UE_LOG(LogInventory, Log, TEXT("=== Network Replication Validation ==="));

	AActor* Owner = Inventory->GetOwner();
	if (Owner)
	{
		if (Owner->GetIsReplicated())
		{
			UE_LOG(LogInventory, Log, TEXT("  [OK] Owner actor is replicated"));
		}
		else
		{
			UE_LOG(LogInventory, Warning, TEXT("  [WARN] Owner actor is NOT replicated"));
			Issues++;
		}
	}

	TArray<UItemBase*> AllItems = Inventory->GetAllItems();
	int32 NonNetworkableItems = 0;
	for (UItemBase* Item : AllItems)
	{
		if (Item && !Item->IsSupportedForNetworking())
		{
			UE_LOG(LogInventory, Warning, TEXT("  [WARN] Item '%s' does not support networking"),
			       *Item->GetItemDefinition().GetItemName().ToString());
			NonNetworkableItems++;
		}
	}
	if (NonNetworkableItems == 0)
	{
		UE_LOG(LogInventory, Log, TEXT("  [OK] All %d items support networking"), AllItems.Num());
	}
	else
	{
		Issues += NonNetworkableItems;
	}

	int32 InvalidOuterCount = 0;
	for (UItemBase* Item : AllItems)
	{
		if (Item)
		{
			UObject* ItemOuter = Item->GetOuter();
			if (!ItemOuter || !ItemOuter->IsSupportedForNetworking())
			{
				UE_LOG(LogInventory, Warning, TEXT("  [WARN] Item '%s' has invalid or non-networkable outer"),
				       *Item->GetItemDefinition().GetItemName().ToString());
				InvalidOuterCount++;
			}
		}
	}
	if (InvalidOuterCount == 0)
	{
		UE_LOG(LogInventory, Log, TEXT("  [OK] All items have valid networkable outers"));
	}
	else
	{
		Issues += InvalidOuterCount;
	}

	int32 NonNetworkableModules = 0;
	for (UItemBase* Item : AllItems)
	{
		if (Item)
		{
			for (UItemModuleBase* Module : Item->GetAllModules())
			{
				if (Module && !Module->IsSupportedForNetworking())
				{
					UE_LOG(LogInventory, Warning, TEXT("  [WARN] Module '%s' on item '%s' does not support networking"),
					       *Module->GetName(), *Item->GetItemDefinition().GetItemName().ToString());
					NonNetworkableModules++;
				}
			}
		}
	}
	if (NonNetworkableModules == 0)
	{
		UE_LOG(LogInventory, Log, TEXT("  [OK] All item modules support networking"));
	}
	else
	{
		Issues += NonNetworkableModules;
	}

	UE_LOG(LogInventory, Log, TEXT("=== Replication Validation Complete: %d issue(s) found ==="), Issues);
}

UInventoryComponent* UInventoryDebugSubsystem::GetPlayerInventory(int32 PlayerIndex)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, PlayerIndex);
	if (!PC || !PC->GetPawn())
	{
		return nullptr;
	}

	return PC->GetPawn()->FindComponentByClass<UInventoryComponent>();
}

void UInventoryDebugSubsystem::RegisterItemClass(TSubclassOf<UItemBase> ItemClass)
{
	if (ItemClass && !RegisteredItemClasses.Contains(ItemClass))
	{
		RegisteredItemClasses.Add(ItemClass);
	}
}

void UInventoryDebugSubsystem::RecordOperation(EInventoryOperationType Type, const FInventoryOperationResult& Result,
                                               float DurationMs, const FString& Context)
{
	OperationTracker.RecordOperation(Type, Result, DurationMs, Context);

	if (bIsProfiling)
	{
		ProfiledOperations++;
	}

#if !UE_BUILD_SHIPPING
	if (bFrameTrackingEnabled)
	{
		CurrentFrameCostMs += DurationMs;
		CurrentFrameOpCount++;
	}
#endif
}

void UInventoryDebugSubsystem::SetOperationTracking(bool bEnabled)
{
	OperationTracker.SetTracking(bEnabled);
	UE_LOG(LogInventory, Log, TEXT("Operation tracking %s"), bEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));
}

TArray<FInventoryOperationRecord> UInventoryDebugSubsystem::GetRecentOperations(int32 Count)
{
	return OperationTracker.GetRecentOperations(Count);
}

TArray<FInventoryOperationRecord> UInventoryDebugSubsystem::GetFailedOperations(int32 Count)
{
	return OperationTracker.GetFailedOperations(Count);
}

FString UInventoryDebugSubsystem::GetOperationSummary()
{
	return OperationTracker.GetSummaryString();
}

void UInventoryDebugSubsystem::SetPerformanceThresholds(float WarningMs, float CriticalMs)
{
	FInventoryPerformanceThresholds Thresholds;
	Thresholds.WarningMs = WarningMs;
	Thresholds.CriticalMs = CriticalMs;
	OperationTracker.SetPerformanceThresholds(Thresholds);
	UE_LOG(LogInventory, Log, TEXT("Performance thresholds set: Warning=%.3fms, Critical=%.3fms"), WarningMs,
	       CriticalMs);
}

TArray<FInventoryPerformanceAlert> UInventoryDebugSubsystem::GetRecentPerformanceAlerts(int32 Count)
{
	return OperationTracker.GetRecentAlerts(Count);
}

void UInventoryDebugSubsystem::SetFrameTracking(bool bEnabled)
{
#if !UE_BUILD_SHIPPING
	if (bEnabled == bFrameTrackingEnabled)
	{
		return;
	}

	bFrameTrackingEnabled = bEnabled;

	if (bEnabled)
	{
		FrameCostHistory.SetNum(FrameCostHistorySize);
		FMemory::Memzero(FrameCostHistory.GetData(), FrameCostHistorySize * sizeof(float));
		FrameCostHistoryIndex = 0;
		FrameCostHistoryCount = 0;
		CurrentFrameCostMs = 0.0f;
		CurrentFrameOpCount = 0;
		PeakFrameCostMs = 0.0f;

		TickDelegateHandle = FWorldDelegates::OnWorldTickStart.AddUObject(
			this, &UInventoryDebugSubsystem::OnWorldTickStart);
		UE_LOG(LogInventory, Log, TEXT("Frame tracking ENABLED"));
	}
	else
	{
		if (TickDelegateHandle.IsValid())
		{
			FWorldDelegates::OnWorldTickStart.Remove(TickDelegateHandle);
			TickDelegateHandle.Reset();
		}
		UE_LOG(LogInventory, Log, TEXT("Frame tracking DISABLED"));
	}
#endif
}

float UInventoryDebugSubsystem::GetCurrentFrameCostMs() const
{
#if !UE_BUILD_SHIPPING
	return CurrentFrameCostMs;
#else
	return 0.0f;
#endif
}

float UInventoryDebugSubsystem::GetAverageFrameCostMs() const
{
#if !UE_BUILD_SHIPPING
	if (FrameCostHistoryCount == 0)
	{
		return 0.0f;
	}

	int32 Count = FMath::Min(FrameCostHistoryCount, FrameCostHistorySize);
	float Total = 0.0f;
	for (int32 i = 0; i < Count; ++i)
	{
		Total += FrameCostHistory[i];
	}
	return Total / Count;
#else
	return 0.0f;
#endif
}

float UInventoryDebugSubsystem::GetPeakFrameCostMs() const
{
#if !UE_BUILD_SHIPPING
	return PeakFrameCostMs;
#else
	return 0.0f;
#endif
}

#if !UE_BUILD_SHIPPING
void UInventoryDebugSubsystem::OnWorldTickStart(UWorld* World, ELevelTick TickType, float DeltaSeconds)
{
	if (!bFrameTrackingEnabled)
	{
		return;
	}

	if (FrameCostHistory.Num() > 0)
	{
		FrameCostHistory[FrameCostHistoryIndex] = CurrentFrameCostMs;
		FrameCostHistoryIndex = (FrameCostHistoryIndex + 1) % FrameCostHistorySize;
		FrameCostHistoryCount++;
	}

	if (CurrentFrameCostMs > PeakFrameCostMs)
	{
		PeakFrameCostMs = CurrentFrameCostMs;
	}

	CurrentFrameCostMs = 0.0f;
	CurrentFrameOpCount = 0;
}
#endif
