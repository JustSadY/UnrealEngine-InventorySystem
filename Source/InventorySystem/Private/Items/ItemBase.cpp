#include "Items/ItemBase.h"
#include "InventorySystem.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Serialization/ArchiveSaveCompressedProxy.h"
#include "InventoryComponent.h"
#include "Types/ItemSaveData.h"

UItemBase::UItemBase()
{
	ItemDefinition = FItemDefinition();
	bIsStackable = false;
	MaxStackSize = 1;
	CurrentStackSize = 1;
	bIsInInventory = false;
}

void UItemBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UItemBase, CurrentStackSize);
	DOREPLIFETIME(UItemBase, bIsInInventory);
	DOREPLIFETIME(UItemBase, OwnerActor);
	DOREPLIFETIME(UItemBase, OwnerInventoryComponent);
	DOREPLIFETIME(UItemBase, ItemModules);

	DOREPLIFETIME_CONDITION(UItemBase, ItemDefinition, COND_InitialOnly);
}

bool UItemBase::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = false;

	for (UItemModuleBase* Module : ItemModules)
	{
		if (Module)
		{
			bWroteSomething |= Channel->ReplicateSubobject(Module, *Bunch, *RepFlags);
		}
	}

	return bWroteSomething;
}

UWorld* UItemBase::GetWorld() const
{
	if (IsTemplate())
		return nullptr;

	if (GetOuter() && GetOuter()->GetWorld())
		return GetOuter()->GetWorld();

	return nullptr;
}

FItemSaveData UItemBase::SaveToStruct(bool bCompress)
{
	FItemSaveData Data;
	Data.ItemID = ItemDefinition.GetItemID();
	Data.ItemClass = GetClass();
	Data.StackSize = CurrentStackSize;

	FMemoryWriter MemoryWriter(Data.ByteData, true);
	FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, true);
	Ar.ArIsSaveGame = true;
	Serialize(Ar);

	if (bCompress && Data.ByteData.Num() > 512)
	{
		TArray<uint8> CompressedData;
		FArchiveSaveCompressedProxy Compressor(CompressedData, NAME_Zlib);

		Compressor << Data.ByteData;
		Compressor.Close();

		if (CompressedData.Num() < Data.ByteData.Num())
		{
			Data.ByteData = MoveTemp(CompressedData);
		}
	}

	return Data;
}

void UItemBase::LoadFromStruct(const FItemSaveData& Data)
{
	ItemDefinition.SetItemID(Data.ItemID);
	CurrentStackSize = Data.StackSize;

	FMemoryReader MemoryReader(Data.ByteData, true);
	FObjectAndNameAsStringProxyArchive Ar(MemoryReader, true);
	Ar.ArIsSaveGame = true;
	Serialize(Ar);
}

void UItemBase::InitializeItem_Implementation()
{
	if (bIsStackable)
	{
		MaxStackSize = FMath::Max(1, MaxStackSize);
		CurrentStackSize = FMath::Clamp(CurrentStackSize, 1, MaxStackSize);
	}
	else
	{
		MaxStackSize = 1;
		CurrentStackSize = 1;
	}

	if (ItemDefinition.GetItemID().IsEmpty())
	{
		GetMutableItemDefinition().SetItemID(GenerateUniqueItemID());
	}

	if (ItemDefinition.GetItemName().IsEmpty())
	{
		GetMutableItemDefinition().SetItemName(FText::FromString(GetClass()->GetName()));
	}

	for (UItemModuleBase* Module : ItemModules)
	{
		if (Module)
		{
			Module->Initialize(this);
		}
	}
}

void UItemBase::SetCurrentStackSize(int32 NewSize)
{
	if (bIsStackable)
	{
		CurrentStackSize = FMath::Clamp(NewSize, 0, MaxStackSize);
	}
	else
	{
		CurrentStackSize = 1;
		if (NewSize != 1)
		{
			UE_LOG(LogInventory, Warning, TEXT("Attempted to set stack size %d on non-stackable item"), NewSize);
		}
	}
}

bool UItemBase::CanMergeWith(const UItemBase* OtherItem) const
{
	if (!OtherItem || !bIsStackable)
		return false;

	return GetItemDefinition().GetItemID() == OtherItem->GetItemDefinition().GetItemID()
		&& CurrentStackSize < MaxStackSize;
}

FInventoryOperationResult UItemBase::MergeWith(UItemBase* OtherItem)
{
	if (!CanMergeWith(OtherItem))
		return FInventoryOperationResult::Fail(TEXT("Cannot merge: items incompatible"));

	int32 SpaceLeft = MaxStackSize - CurrentStackSize;
	int32 TransferAmount = FMath::Min(SpaceLeft, OtherItem->CurrentStackSize);

	SetCurrentStackSize(CurrentStackSize + TransferAmount);
	OtherItem->SetCurrentStackSize(OtherItem->CurrentStackSize - TransferAmount);

	for (UItemModuleBase* Module : ItemModules)
	{
		if (Module && Module->IsModuleActive())
		{
			Module->OnItemMerged(OtherItem, false);
		}
	}

	return FInventoryOperationResult::Ok();
}

UItemBase* UItemBase::SplitStack(int32 Amount)
{
	if (!bIsStackable)
	{
		UE_LOG(LogInventory, Warning, TEXT("Cannot split non-stackable item"));
		return nullptr;
	}

	if (Amount <= 0)
	{
		UE_LOG(LogInventory, Warning, TEXT("Cannot split with amount <= 0"));
		return nullptr;
	}

	if (Amount >= CurrentStackSize)
	{
		UE_LOG(LogInventory, Warning, TEXT("Cannot split %d from stack of %d (must leave at least 1)"), Amount,
		       CurrentStackSize);
		return nullptr;
	}

	CurrentStackSize -= Amount;

	UItemBase* NewItem = NewObject<UItemBase>(GetTransientPackage(), GetClass());
	if (!NewItem)
	{
		UE_LOG(LogInventory, Error, TEXT("Failed to create new item instance for split"));
		CurrentStackSize += Amount;
		return nullptr;
	}

	CopyDefinitionTo(NewItem);

	NewItem->bIsStackable = bIsStackable;
	NewItem->MaxStackSize = MaxStackSize;
	NewItem->SetCurrentStackSize(Amount);

	for (UItemModuleBase* Module : ItemModules)
	{
		if (Module)
		{
			if (UItemModuleBase* NewModule = Module->DuplicateModule(NewItem))
			{
				NewItem->ItemModules.Add(NewModule);
				if (Module->IsModuleActive())
				{
					Module->OnItemSplit(NewItem, Amount);
				}
			}
		}
	}

	NewItem->AddToRoot();

	return NewItem;
}

void UItemBase::OnAddedToInventory(AActor* NewOwner)
{
	if (!NewOwner)
		return;

	OwnerActor = NewOwner;
	bIsInInventory = true;

	if (IsRooted())
	{
		RemoveFromRoot();
	}

	for (UItemModuleBase* Module : ItemModules)
	{
		if (Module && Module->IsModuleActive())
		{
			Module->OnItemAddedToInventory(NewOwner);
		}
	}
}

void UItemBase::OnRemovedFromInventory()
{
	for (UItemModuleBase* Module : ItemModules)
	{
		if (Module && Module->IsModuleActive())
		{
			Module->OnItemRemovedFromInventory();
		}
	}

	bIsInInventory = false;
	OwnerActor = nullptr;
	OwnerInventoryComponent = nullptr;
}

FInventoryOperationResult UItemBase::AddModule(UItemModuleBase* NewModule)
{
	if (!NewModule)
		return FInventoryOperationResult::Fail(TEXT("Invalid module"));

	if (GetModuleByClass(NewModule->GetClass()))
		return FInventoryOperationResult::Fail(TEXT("Module of this class already exists on item"));

	ItemModules.Add(NewModule);
	NewModule->Initialize(this);

	InvalidateModuleCache();

	if (bIsInInventory)
		NewModule->OnItemAddedToInventory(OwnerActor);

	return FInventoryOperationResult::Ok();
}

FInventoryOperationResult UItemBase::RemoveModule(UItemModuleBase* ModuleToRemove)
{
	if (!ModuleToRemove)
		return FInventoryOperationResult::Fail(TEXT("Invalid module"));

	if (!ItemModules.Contains(ModuleToRemove))
		return FInventoryOperationResult::Fail(TEXT("Module not found on item"));

	if (bIsInInventory)
		ModuleToRemove->OnItemRemovedFromInventory();

	ItemModules.Remove(ModuleToRemove);
	InvalidateModuleCache();

	return FInventoryOperationResult::Ok();
}

UItemModuleBase* UItemBase::GetModuleByClass(TSubclassOf<UItemModuleBase> ModuleClass) const
{
	if (!ModuleClass)
		return nullptr;

	for (UItemModuleBase* Module : ItemModules)
	{
		if (Module && Module->IsA(ModuleClass))
			return Module;
	}
	return nullptr;
}

bool UItemBase::Validate(TArray<FString>& OutErrors) const
{
	bool bIsValid = true;

	TArray<FString> DefinitionErrors;
	if (!ItemDefinition.Validate(DefinitionErrors))
	{
		OutErrors.Append(DefinitionErrors);
		bIsValid = false;
	}

	if (bIsStackable)
	{
		if (MaxStackSize < 1)
		{
			OutErrors.Add(FString::Printf(TEXT("Invalid MaxStackSize: %d (must be >= 1)"), MaxStackSize));
			bIsValid = false;
		}

		if (CurrentStackSize < 0 || CurrentStackSize > MaxStackSize)
		{
			OutErrors.Add(
				FString::Printf(TEXT("Invalid CurrentStackSize: %d (must be 0-%d)"), CurrentStackSize, MaxStackSize));
			bIsValid = false;
		}
	}
	else
	{
		if (CurrentStackSize != 1)
		{
			OutErrors.Add(FString::Printf(TEXT("Non-stackable item has stack size %d"), CurrentStackSize));
			bIsValid = false;
		}
	}

	for (int32 i = 0; i < ItemModules.Num(); ++i)
	{
		if (!ItemModules[i])
		{
			OutErrors.Add(FString::Printf(TEXT("Null module at index %d"), i));
			bIsValid = false;
		}
	}

	return bIsValid;
}

FString UItemBase::GetDebugString() const
{
	return FString::Printf(TEXT("Item[%s] Stack:%d/%d InInventory:%s Owner:%s"),
	                       *ItemDefinition.GetItemName().ToString(),
	                       CurrentStackSize,
	                       MaxStackSize,
	                       bIsInInventory ? TEXT("Yes") : TEXT("No"),
	                       OwnerActor ? *OwnerActor->GetName() : TEXT("None"));
}

FString UItemBase::GenerateUniqueItemID() const
{
	return FGuid::NewGuid().ToString(EGuidFormats::Digits);
}

void UItemBase::CopyDefinitionTo(UItemBase* TargetItem) const
{
	if (!TargetItem)
		return;

	FItemDefinition& Target = TargetItem->GetMutableItemDefinition();

	Target.SetItemID(ItemDefinition.GetItemID());
	Target.SetItemName(ItemDefinition.GetItemName());
	Target.SetItemDescription(ItemDefinition.GetItemDescription());
	Target.SetItemIcon(ItemDefinition.GetItemIcon());
	Target.SetInventorySlotTypeIDs(ItemDefinition.GetInventorySlotTypeIDs());
}
