#include "QuickAccessSlots.h"
#include "InventorySystem.h"
#include "InventoryDebugSubsystem.h"
#include "Net/UnrealNetwork.h"

UQuickAccessSlots::UQuickAccessSlots()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UQuickAccessSlots::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UQuickAccessSlots, QuickSlots);
}

void UQuickAccessSlots::BeginPlay()
{
	Super::BeginPlay();
	InitializeQuickSlots();
}

void UQuickAccessSlots::InitializeQuickSlots()
{
	QuickSlots.Empty();
	QuickSlots.SetNum(MaxQuickSlots);
}

FInventoryOperationResult UQuickAccessSlots::AssignToQuickSlot(int32 SlotIndex, UItemBase* Item, int32 SourceGroupIndex, int32 SourceSlotIndex)
{
	double StartTime = FPlatformTime::Seconds();

	if (!IsValidSlotIndex(SlotIndex))
	{
		UE_LOG(LogInventory, Warning, TEXT("AssignToQuickSlot: Invalid slot index %d"), SlotIndex);
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(FString::Printf(TEXT("Invalid slot index %d"), SlotIndex));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_QuickSlotAssign, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0),
			FString::Printf(TEXT("Invalid index %d"), SlotIndex));
		return FailResult;
	}

	if (QuickSlots[SlotIndex].bIsLocked)
	{
		UE_LOG(LogInventory, Warning, TEXT("AssignToQuickSlot: Slot %d is locked"), SlotIndex);
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(FString::Printf(TEXT("Slot %d is locked"), SlotIndex));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_QuickSlotAssign, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0),
			FString::Printf(TEXT("Slot %d locked"), SlotIndex));
		return FailResult;
	}

	if (!bAllowDuplicates && Item != nullptr && IsInQuickSlot(Item))
	{
		UE_LOG(LogInventory, Warning, TEXT("AssignToQuickSlot: Item already in quick slot and duplicates not allowed"));
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(TEXT("Item already in quick slot and duplicates not allowed"));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_QuickSlotAssign, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0), TEXT("Duplicate not allowed"));
		return FailResult;
	}

	QuickSlots[SlotIndex].Item = Item;
	QuickSlots[SlotIndex].SourceGroupIndex = SourceGroupIndex;
	QuickSlots[SlotIndex].SourceSlotIndex = SourceSlotIndex;

	OnQuickSlotChanged.Broadcast(SlotIndex, Item);

	FInventoryOperationResult OkResult = FInventoryOperationResult::Ok();
	TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_QuickSlotAssign, OkResult,
		static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0),
		FString::Printf(TEXT("Slot:%d"), SlotIndex));
	return OkResult;
}

FInventoryOperationResult UQuickAccessSlots::ClearQuickSlot(int32 SlotIndex)
{
	double StartTime = FPlatformTime::Seconds();

	if (!IsValidSlotIndex(SlotIndex))
	{
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(FString::Printf(TEXT("Invalid slot index %d"), SlotIndex));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_QuickSlotClear, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0),
			FString::Printf(TEXT("Invalid index %d"), SlotIndex));
		return FailResult;
	}

	if (QuickSlots[SlotIndex].bIsLocked)
	{
		UE_LOG(LogInventory, Warning, TEXT("ClearQuickSlot: Slot %d is locked"), SlotIndex);
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(FString::Printf(TEXT("Slot %d is locked"), SlotIndex));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_QuickSlotClear, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0),
			FString::Printf(TEXT("Slot %d locked"), SlotIndex));
		return FailResult;
	}

	QuickSlots[SlotIndex].Clear();
	OnQuickSlotChanged.Broadcast(SlotIndex, nullptr);

	FInventoryOperationResult OkResult = FInventoryOperationResult::Ok();
	TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_QuickSlotClear, OkResult,
		static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0),
		FString::Printf(TEXT("Slot:%d"), SlotIndex));
	return OkResult;
}

UItemBase* UQuickAccessSlots::GetQuickSlotItem(int32 SlotIndex) const
{
	if (!IsValidSlotIndex(SlotIndex))
	{
		return nullptr;
	}

	return QuickSlots[SlotIndex].Item;
}

FQuickSlot UQuickAccessSlots::GetQuickSlot(int32 SlotIndex) const
{
	if (!IsValidSlotIndex(SlotIndex))
	{
		return FQuickSlot();
	}

	return QuickSlots[SlotIndex];
}

FInventoryOperationResult UQuickAccessSlots::UseQuickSlot(int32 SlotIndex)
{
	double StartTime = FPlatformTime::Seconds();

	if (!IsValidSlotIndex(SlotIndex))
	{
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(FString::Printf(TEXT("Invalid slot index %d"), SlotIndex));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_QuickSlotUse, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0),
			FString::Printf(TEXT("Invalid index %d"), SlotIndex));
		return FailResult;
	}

	UItemBase* Item = QuickSlots[SlotIndex].Item;
	if (!IsValid(Item))
	{
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(FString::Printf(TEXT("Slot %d has no valid item"), SlotIndex));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_QuickSlotUse, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0),
			FString::Printf(TEXT("Slot %d no item"), SlotIndex));
		return FailResult;
	}

	OnQuickSlotUsed.Broadcast(SlotIndex, Item);

	FInventoryOperationResult OkResult = FInventoryOperationResult::Ok();
	TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_QuickSlotUse, OkResult,
		static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0),
		FString::Printf(TEXT("Slot:%d Item:%s"), SlotIndex, *Item->GetClass()->GetName()));
	return OkResult;
}

FInventoryOperationResult UQuickAccessSlots::SwapQuickSlots(int32 SlotIndexA, int32 SlotIndexB)
{
	double StartTime = FPlatformTime::Seconds();

	if (!IsValidSlotIndex(SlotIndexA) || !IsValidSlotIndex(SlotIndexB))
	{
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(TEXT("One or both slot indices are invalid"));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_QuickSlotSwap, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0),
			FString::Printf(TEXT("Invalid indices %d,%d"), SlotIndexA, SlotIndexB));
		return FailResult;
	}

	if (QuickSlots[SlotIndexA].bIsLocked || QuickSlots[SlotIndexB].bIsLocked)
	{
		UE_LOG(LogInventory, Warning, TEXT("SwapQuickSlots: One or both slots are locked"));
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(TEXT("One or both slots are locked"));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_QuickSlotSwap, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0), TEXT("Slots locked"));
		return FailResult;
	}

	QuickSlots.Swap(SlotIndexA, SlotIndexB);

	OnQuickSlotChanged.Broadcast(SlotIndexA, QuickSlots[SlotIndexA].Item);
	OnQuickSlotChanged.Broadcast(SlotIndexB, QuickSlots[SlotIndexB].Item);

	FInventoryOperationResult OkResult = FInventoryOperationResult::Ok();
	TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_QuickSlotSwap, OkResult,
		static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0),
		FString::Printf(TEXT("Swap %d<->%d"), SlotIndexA, SlotIndexB));
	return OkResult;
}

int32 UQuickAccessSlots::FindQuickSlot(UItemBase* Item) const
{
	if (!IsValid(Item))
	{
		return -1;
	}

	for (int32 i = 0; i < QuickSlots.Num(); ++i)
	{
		if (QuickSlots[i].Item == Item)
		{
			return i;
		}
	}

	return -1;
}

bool UQuickAccessSlots::IsInQuickSlot(UItemBase* Item) const
{
	return FindQuickSlot(Item) != -1;
}

void UQuickAccessSlots::LockQuickSlot(int32 SlotIndex, bool bLocked)
{
	if (!IsValidSlotIndex(SlotIndex))
	{
		return;
	}

	QuickSlots[SlotIndex].bIsLocked = bLocked;
}

void UQuickAccessSlots::ClearAllQuickSlots()
{
	for (int32 i = 0; i < QuickSlots.Num(); ++i)
	{
		if (!QuickSlots[i].bIsLocked)
		{
			QuickSlots[i].Clear();
			OnQuickSlotChanged.Broadcast(i, nullptr);
		}
	}
}

void UQuickAccessSlots::SetQuickSlotKeyBinding(int32 SlotIndex, FName KeyBinding)
{
	if (!IsValidSlotIndex(SlotIndex))
	{
		return;
	}

	QuickSlots[SlotIndex].KeyBinding = KeyBinding;
}

bool UQuickAccessSlots::IsValidSlotIndex(int32 SlotIndex) const
{
	return QuickSlots.IsValidIndex(SlotIndex);
}

void UQuickAccessSlots::OnItemRemovedFromInventory(UItemBase* Item)
{
	if (!bAutoClearOnItemRemoved || !IsValid(Item))
	{
		return;
	}

	for (int32 i = 0; i < QuickSlots.Num(); ++i)
	{
		if (QuickSlots[i].Item == Item && !QuickSlots[i].bIsLocked)
		{
			ClearQuickSlot(i);
		}
	}
}
