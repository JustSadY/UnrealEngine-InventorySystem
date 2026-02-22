#include "Modules/InventoryModuleBase.h"

#include "InventoryComponent.h"

UInventoryModuleBase::UInventoryModuleBase()
{
	ModuleType = FName("General");
	bWantsTick = false;
}

void UInventoryModuleBase::InitializeModule_Implementation()
{
	// Default implementation
}

void UInventoryModuleBase::BeginPlay_Implementation()
{
	// Default implementation
}

void UInventoryModuleBase::Tick_Implementation(float DeltaTime)
{
	// Default implementation
}

void UInventoryModuleBase::OnModuleInstalled_Implementation(UInventoryComponent* ParentInventoryComponent)
{
	if (!ParentInventoryComponent) return;
	OwningInventory = ParentInventoryComponent;
	ParentInventoryComponent->OnItemAdded.AddDynamic(this, &UInventoryModuleBase::OnItemAdded);
	ParentInventoryComponent->OnItemRemoved.AddDynamic(this, &UInventoryModuleBase::OnItemRemoved);
}

void UInventoryModuleBase::OnModuleRemoved_Implementation()
{
	if (!OwningInventory) return;
	OwningInventory->OnItemAdded.RemoveDynamic(this, &UInventoryModuleBase::OnItemAdded);
	OwningInventory->OnItemRemoved.RemoveDynamic(this, &UInventoryModuleBase::OnItemRemoved);
	OwningInventory = nullptr;
}
