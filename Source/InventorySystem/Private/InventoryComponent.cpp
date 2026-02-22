#include "InventoryComponent.h"
#include "InventorySystem.h"
#include "InventoryDebugSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "Modules/InventoryModuleBase.h"
#include "PoolSystem/ItemPoolSubsystem.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bAutoStackItems = true;
	SetIsReplicatedByDefault(true);
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UInventoryComponent, InventorySlotsGroup);
	DOREPLIFETIME(UInventoryComponent, InstalledModules);
}

bool UInventoryComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (const FInventorySlots& Group : InventorySlotsGroup.GetInventoryGroups())
	{
		for (const FInventorySlot& Slot : Group.GetSlots())
		{
			if (IsValid(Slot.GetItem()))
			{
				bWroteSomething |= Channel->ReplicateSubobject(Slot.GetItem(), *Bunch, *RepFlags);
			}
		}
	}

	for (UInventoryModuleBase* Module : InstalledModules)
	{
		if (IsValid(Module))
		{
			bWroteSomething |= Channel->ReplicateSubobject(Module, *Bunch, *RepFlags);
		}
	}

	return bWroteSomething;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	for (FInventorySlots& Group : InventorySlotsGroup.GetInventoryGroups())
	{
		if (Group.GetSlots().Num() == 0 && Group.GetMaxSlotSize() > 0)
		{
			Group.InitializeInventory(Group.GetMaxSlotSize(), Group.GetTypeIDMap());
		}
	}

	InventorySlotsGroup.RebuildCache();
}

void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	for (UInventoryModuleBase* Module : InstalledModules)
	{
		if (IsValid(Module) && Module->GetWantsTick())
		{
			Module->Tick(DeltaTime);
		}
	}
}

FInventoryOperationResult UInventoryComponent::AddItem(UItemBase* Item, int32 TargetTypeID)
{
	double StartTime = FPlatformTime::Seconds();

	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogInventory, Warning, TEXT("AddItem: No authority or no owner"));
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(TEXT("No authority or no owner"));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_AddItem, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0), TEXT("No authority"));
		return FailResult;
	}

	if (!IsValid(Item))
	{
		UE_LOG(LogInventory, Error, TEXT("AddItem: Invalid item"));
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(TEXT("Invalid item"));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_AddItem, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0), TEXT("Invalid item"));
		return FailResult;
	}

	FInventoryOperationResult Result = InventorySlotsGroup.AddItem(Item, TargetTypeID);

	if (Result.bSuccess)
	{
		int32 FoundTypeID, SlotIdx;
		if (FindItemLocation(Item, FoundTypeID, SlotIdx))
		{
			Item->OnAddedToInventory(GetOwner());
			OnItemAdded.Broadcast(Item, FoundTypeID, SlotIdx);
		}
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_AddItem, Result,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0),
			FString::Printf(TEXT("Item: %s"), *Item->GetClass()->GetName()));
		return Result;
	}

	UE_LOG(LogInventory, Warning, TEXT("AddItem failed: %s"), *Result.Message);
	OnInventoryFull.Broadcast(Item, 1);
	TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_AddItem, Result,
		static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0),
		FString::Printf(TEXT("Failed: %s"), *Result.Message));
	return Result;
}

FInventoryOperationResult UInventoryComponent::RemoveItemAt(int32 TypeID, int32 SlotIndex, int32 Quantity)
{
	double StartTime = FPlatformTime::Seconds();

	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogInventory, Warning, TEXT("RemoveItemAt: No authority or no owner"));
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(TEXT("No authority or no owner"));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_RemoveItemAt, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0), TEXT("No authority"));
		return FailResult;
	}

	if (Quantity <= 0)
	{
		UE_LOG(LogInventory, Error, TEXT("RemoveItemAt: Invalid quantity %d"), Quantity);
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(TEXT("Invalid quantity"));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_RemoveItemAt, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0), TEXT("Invalid quantity"));
		return FailResult;
	}

	FInventorySlots* TargetGroup = InventorySlotsGroup.GetGroupByID(TypeID);
	if (!TargetGroup)
	{
		UE_LOG(LogInventory, Error, TEXT("RemoveItemAt: Target group with TypeID %d not found"), TypeID);
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(FString::Printf(TEXT("Target group with TypeID %d not found"), TypeID));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_RemoveItemAt, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0),
			FString::Printf(TEXT("Group %d not found"), TypeID));
		return FailResult;
	}

	const FInventorySlot* Slot = TargetGroup->GetSlotAtIndex(SlotIndex);
	if (!Slot || Slot->IsEmpty())
	{
		UE_LOG(LogInventory, Warning, TEXT("RemoveItemAt: Slot %d in group %d is empty or invalid"), SlotIndex, TypeID);
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(FString::Printf(TEXT("Slot %d in group %d is empty or invalid"), SlotIndex, TypeID));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_RemoveItemAt, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0),
			FString::Printf(TEXT("Slot %d empty"), SlotIndex));
		return FailResult;
	}

	UItemBase* ItemRef = Slot->GetItem();

	FInventoryOperationResult RemoveResult = TargetGroup->RemoveStackAmountFromSlot(SlotIndex, Quantity);
	if (RemoveResult.bSuccess)
	{
		if (Slot->IsEmpty())
		{
			ItemRef->OnRemovedFromInventory();

			if (UWorld* World = GetWorld())
			{
				if (UItemPoolSubsystem* PoolSubsystem = World->GetSubsystem<UItemPoolSubsystem>())
				{
					PoolSubsystem->ReturnItemToPool(ItemRef);
				}
			}
		}

		OnItemRemoved.Broadcast(ItemRef, TypeID, SlotIndex);
		FInventoryOperationResult OkResult = FInventoryOperationResult::Ok();
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_RemoveItemAt, OkResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0),
			FString::Printf(TEXT("Group:%d Slot:%d Qty:%d"), TypeID, SlotIndex, Quantity));
		return OkResult;
	}

	TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_RemoveItemAt, RemoveResult,
		static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0),
		FString::Printf(TEXT("Failed: %s"), *RemoveResult.Message));
	return RemoveResult;
}

FInventoryOperationResult UInventoryComponent::TransferItem(int32 FromTypeID, int32 FromIndex, int32 ToTypeID, int32 ToIndex)
{
	double StartTime = FPlatformTime::Seconds();

	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogInventory, Warning, TEXT("TransferItem: No authority or no owner"));
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(TEXT("No authority or no owner"));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_TransferItem, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0), TEXT("No authority"));
		return FailResult;
	}

	FInventoryOperationResult Result = InventorySlotsGroup.TransferItem(FromTypeID, FromIndex, ToTypeID, ToIndex);

	if (!Result.bSuccess)
	{
		UE_LOG(LogInventory, Warning, TEXT("TransferItem failed: %s (From Group %d Index %d to Group %d Index %d)"),
		       *Result.Message, FromTypeID, FromIndex, ToTypeID, ToIndex);
	}

	TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_TransferItem, Result,
		static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0),
		FString::Printf(TEXT("From:%d,%d To:%d,%d"), FromTypeID, FromIndex, ToTypeID, ToIndex));
	return Result;
}

bool UInventoryComponent::FindItemLocation(UItemBase* Item, int32& OutTypeID, int32& OutSlotIndex) const
{
	return InventorySlotsGroup.FindItemLocation(Item, OutTypeID, OutSlotIndex);
}

void UInventoryComponent::OrganizeInventory()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	
	InventorySlotsGroup.OrganizeAll();
}

bool UInventoryComponent::CanAddItem(UItemBase* Item, int32 SlotTypeID) const
{
	if (!IsValid(Item))
	{
		return false;
	}

	if (bAutoStackItems && Item->IsStackable())
	{
		const TArray<FInventorySlots>& Groups = InventorySlotsGroup.GetInventoryGroups();
		for (const FInventorySlots& Group : Groups)
		{
			if (SlotTypeID != -1 && !Group.GetTypeIDMap().Contains(SlotTypeID))
			{
				continue;
			}
			
			if (!Group.IsTypeSupported(Item))
			{
				continue;
			}

			for (const FInventorySlot& Slot : Group.GetSlots())
			{
				if (!Slot.IsEmpty() && 
					Slot.GetItem()->GetItemDefinition().GetItemID() == Item->GetItemDefinition().GetItemID())
				{
					if (!Slot.IsFull())
					{
						return true;
					}
				}
			}
		}
	}

	return GetEmptySlotCount(SlotTypeID) > 0;
}

int32 UInventoryComponent::GetEmptySlotCount(int32 SlotTypeID) const
{
	int32 TotalEmpty = 0;
	const TArray<FInventorySlots>& Groups = InventorySlotsGroup.GetInventoryGroups();
	
	for (const FInventorySlots& Group : Groups)
	{
		if (SlotTypeID != -1 && !Group.GetTypeIDMap().Contains(SlotTypeID))
		{
			continue;
		}
		TotalEmpty += Group.GetFreeSlotCount();
	}
	
	return TotalEmpty;
}

FInventoryOperationResult UInventoryComponent::TryStackItem(UItemBase* Item, int32 SlotTypeID)
{
	double StartTime = FPlatformTime::Seconds();

	if (!IsValid(Item) || !Item->IsStackable())
	{
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(TEXT("Invalid item or item is not stackable"));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_StackItem, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0), TEXT("Invalid or not stackable"));
		return FailResult;
	}

	const TArray<FInventorySlots>& Groups = InventorySlotsGroup.GetInventoryGroups();

	for (int32 GroupIdx = 0; GroupIdx < Groups.Num(); ++GroupIdx)
	{
		FInventorySlots& Group = InventorySlotsGroup.GetInventoryGroups()[GroupIdx];

		if (!Group.IsTypeSupported(Item))
		{
			continue;
		}

		if (SlotTypeID != -1 && !Group.GetTypeIDMap().Contains(SlotTypeID))
		{
			continue;
		}

		TArray<FInventorySlot>& Slots = Group.GetSlotsMutable();

		for (int32 j = 0; j < Slots.Num(); ++j)
		{
			FInventorySlot& Slot = Slots[j];

			if (!Slot.IsEmpty() &&
				Slot.GetItem()->GetItemDefinition().GetItemID() == Item->GetItemDefinition().GetItemID())
			{
				int32 OldAmount = Slot.GetCurrentStackSize();
				int32 Overflow = Slot.AddToStack(Item->GetCurrentStackSize());

				int32 CurrentTypeID = InventorySlotsGroup.GetTypeIDForGroupIndex(GroupIdx);

				OnItemStackChanged.Broadcast(Slot.GetItem(), CurrentTypeID, j, OldAmount, Slot.GetCurrentStackSize());

				if (Overflow <= 0)
				{
					if (UWorld* World = GetWorld())
					{
						if (UItemPoolSubsystem* PoolSubsystem = World->GetSubsystem<UItemPoolSubsystem>())
						{
							PoolSubsystem->ReturnItemToPool(Item);
						}
						else
						{
							Item->MarkAsGarbage();
						}
					}
					else
					{
						Item->MarkAsGarbage();
					}
					FInventoryOperationResult OkResult = FInventoryOperationResult::Ok();
					TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_StackItem, OkResult,
						static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0),
						FString::Printf(TEXT("Stacked %s"), *Item->GetClass()->GetName()));
					return OkResult;
				}

				Item->SetCurrentStackSize(Overflow);
			}
		}
	}

	FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(TEXT("No compatible stack found for this item"));
	TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_StackItem, FailResult,
		static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0), TEXT("No compatible stack"));
	return FailResult;
}

int32 UInventoryComponent::GetItemCount(UItemBase* Item, int32 SlotTypeID) const
{
	if (!IsValid(Item))
	{
		return 0;
	}

	int32 Count = 0;
	const TArray<FInventorySlots>& Groups = InventorySlotsGroup.GetInventoryGroups();
	
	for (const FInventorySlots& Group : Groups)
	{
		if (SlotTypeID != -1 && !Group.GetTypeIDMap().Contains(SlotTypeID))
		{
			continue;
		}
		
		for (const FInventorySlot& Slot : Group.GetSlots())
		{
			if (!Slot.IsEmpty() && 
				Slot.GetItem()->GetItemDefinition().GetItemID() == Item->GetItemDefinition().GetItemID())
			{
				Count += Slot.GetCurrentStackSize();
			}
		}
	}
	
	return Count;
}

FInventoryOperationResult UInventoryComponent::RemoveItem(UItemBase* Item)
{
	double StartTime = FPlatformTime::Seconds();

	if (!IsValid(Item))
	{
		UE_LOG(LogInventory, Error, TEXT("RemoveItem: Invalid item"));
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(TEXT("Invalid item"));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_RemoveItem, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0), TEXT("Invalid item"));
		return FailResult;
	}

	int32 TypeID, SlotIndex;
	if (FindItemLocation(Item, TypeID, SlotIndex))
	{
		FInventoryOperationResult Result = RemoveItemAt(TypeID, SlotIndex, Item->GetCurrentStackSize());
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_RemoveItem, Result,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0),
			FString::Printf(TEXT("Item: %s"), *Item->GetClass()->GetName()));
		return Result;
	}

	UE_LOG(LogInventory, Warning, TEXT("RemoveItem: Item not found in inventory"));
	FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(TEXT("Item not found in inventory"));
	TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_RemoveItem, FailResult,
		static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0), TEXT("Item not found"));
	return FailResult;
}

UItemBase* UInventoryComponent::GetItemAtIndex(int32 GroupIndex, int32 SlotIndex) const
{
	const FInventorySlots* Slots = InventorySlotsGroup.GetItemsByTypeID(GroupIndex);
	
	if (!Slots)
	{
		return nullptr;
	}

	if (!Slots->GetSlots().IsValidIndex(SlotIndex))
	{
		return nullptr;
	}

	return Slots->GetSlots()[SlotIndex].GetItem();
}

void UInventoryComponent::SortInventory()
{
	OrganizeInventory();
}

void UInventoryComponent::ClearInventory()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	TArray<FInventorySlots>& Groups = InventorySlotsGroup.GetInventoryGroups();
	for (FInventorySlots& Group : Groups)
	{
		for (const FInventorySlot& Slot : Group.GetSlots())
		{
			if (!Slot.IsEmpty() && IsValid(Slot.GetItem()))
			{
				UItemBase* Item = Slot.GetItem();
				Item->OnRemovedFromInventory();
				
				if (UWorld* World = GetWorld())
				{
					if (UItemPoolSubsystem* PoolSubsystem = World->GetSubsystem<UItemPoolSubsystem>())
					{
						PoolSubsystem->ReturnItemToPool(Item);
					}
				}
			}
		}
		Group.ClearAllSlots();
	}
}

TArray<UItemBase*> UInventoryComponent::GetAllItems() const
{
	TArray<UItemBase*> Items;
	Items.Reserve(64);
	
	for (const FInventorySlots& Group : InventorySlotsGroup.GetInventoryGroups())
	{
		for (const FInventorySlot& Slot : Group.GetSlots())
		{
			if (!Slot.IsEmpty())
			{
				Items.Add(Slot.GetItem());
			}
		}
	}
	
	return Items;
}

FInventoryOperationResult UInventoryComponent::AddItemByClass(TSubclassOf<UItemBase> ItemClass, int32 Quantity, int32 SlotTypeID)
{
	double StartTime = FPlatformTime::Seconds();

	if (!ItemClass || Quantity <= 0)
	{
		UE_LOG(LogInventory, Error, TEXT("AddItemByClass: Invalid ItemClass or Quantity"));
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(TEXT("Invalid ItemClass or Quantity"));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_AddItem, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0), TEXT("Invalid class or quantity"));
		return FailResult;
	}

	UItemBase* NewItem = CreateItemInstance(ItemClass);
	if (!IsValid(NewItem))
	{
		UE_LOG(LogInventory, Error, TEXT("AddItemByClass: Failed to create item instance"));
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(TEXT("Failed to create item instance"));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_AddItem, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0), TEXT("Failed to create instance"));
		return FailResult;
	}

	NewItem->SetCurrentStackSize(Quantity);

	FInventoryOperationResult Result = AddItem(NewItem, SlotTypeID);
	if (!Result.bSuccess)
	{
		if (UWorld* World = GetWorld())
		{
			if (UItemPoolSubsystem* PoolSubsystem = World->GetSubsystem<UItemPoolSubsystem>())
			{
				PoolSubsystem->ReturnItemToPool(NewItem);
			}
			else
			{
				NewItem->MarkAsGarbage();
			}
		}
		else
		{
			NewItem->MarkAsGarbage();
		}
		return Result;
	}

	return Result;
}

UItemBase* UInventoryComponent::CreateItemInstance(TSubclassOf<UItemBase> ItemClass)
{
	if (!ItemClass)
	{
		UE_LOG(LogInventory, Error, TEXT("CreateItemInstance: Invalid ItemClass"));
		return nullptr;
	}

	if (UWorld* World = GetWorld())
	{
		if (UItemPoolSubsystem* PoolSubsystem = World->GetSubsystem<UItemPoolSubsystem>())
		{
			if (UItemBase* PooledItem = PoolSubsystem->GetItemFromPool(ItemClass, this))
			{
				return PooledItem;
			}
		}
	}

	UItemBase* NewItem = NewObject<UItemBase>(this, ItemClass);
	
	if (NewItem)
	{
		NewItem->InitializeItem();
	}
	else
	{
		UE_LOG(LogInventory, Error, TEXT("CreateItemInstance: Failed to create NewObject for class %s"), 
			*ItemClass->GetName());
	}

	return NewItem;
}