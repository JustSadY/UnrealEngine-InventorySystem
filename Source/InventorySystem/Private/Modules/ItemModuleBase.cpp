#include "Modules/ItemModuleBase.h"
#include "InventorySystem.h"
#include "Items/ItemBase.h"

UItemModuleBase::UItemModuleBase()
{
	OwnerItem = nullptr;
	bIsModuleActive = true;
	Priority = 100;
}

void UItemModuleBase::Initialize(UItemBase* InOwnerItem)
{
	if (!InOwnerItem)
	{
		UE_LOG(LogInventory, Warning, TEXT("ItemModuleBase::Initialize - Invalid owner item!"));
		return;
	}

	OwnerItem = InOwnerItem;
	bIsModuleActive = true;
}

void UItemModuleBase::Reset()
{
	bIsModuleActive = true;
}

void UItemModuleBase::OnItemAddedToInventory_Implementation(AActor* Owner)
{
	// Base implementation - override in child classes
}

void UItemModuleBase::OnItemRemovedFromInventory_Implementation()
{
	// Base implementation - override in child classes
}

// ================= Stack Events =================

void UItemModuleBase::OnItemMerged_Implementation(UItemBase* OtherItem, bool bIsSource)
{
	// Base implementation - override in child classes
}

void UItemModuleBase::OnItemSplit_Implementation(UItemBase* NewItem, int32 Amount)
{
	// Base implementation - override in child classes
}

TMap<FString, FString> UItemModuleBase::SerializeModuleData_Implementation() const
{
	TMap<FString, FString> Data;
	Data.Add(TEXT("bIsModuleActive"), bIsModuleActive ? TEXT("true") : TEXT("false"));
	Data.Add(TEXT("Priority"), FString::FromInt(Priority));
	return Data;
}

void UItemModuleBase::DeserializeModuleData_Implementation(const TMap<FString, FString>& Data)
{
	if (const FString* ActiveValue = Data.Find(TEXT("bIsModuleActive")))
	{
		bIsModuleActive = (*ActiveValue == TEXT("true"));
	}

	if (const FString* PriorityValue = Data.Find(TEXT("Priority")))
	{
		Priority = FCString::Atoi(**PriorityValue);
	}
}

UItemModuleBase* UItemModuleBase::DuplicateModule(UItemBase* TargetItem)
{
	if (!TargetItem)
	{
		UE_LOG(LogInventory, Warning, TEXT("ItemModuleBase::DuplicateModule - Invalid target item!"));
		return nullptr;
	}

	UItemModuleBase* NewModule = NewObject<UItemModuleBase>(TargetItem, GetClass());

	if (NewModule)
	{
		NewModule->Initialize(TargetItem);
		NewModule->bIsModuleActive = bIsModuleActive;
		NewModule->Priority = Priority;

		TMap<FString, FString> Data = SerializeModuleData();
		NewModule->DeserializeModuleData(Data);
	}

	return NewModule;
}
