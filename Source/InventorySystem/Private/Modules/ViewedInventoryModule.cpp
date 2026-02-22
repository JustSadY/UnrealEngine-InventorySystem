#include "Modules/ViewedInventoryModule.h"
#include "InventoryComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Modules/WearableModule.h"

UViewedInventoryModule::UViewedInventoryModule()
	: ViewSlotCount(1)
	  , ViewSlotTypeID(1)
{
}

void UViewedInventoryModule::OnModuleInstalled_Implementation(UInventoryComponent* ParentInventoryComponent)
{
	Super::OnModuleInstalled_Implementation(ParentInventoryComponent);

	if (!IsValid(ParentInventoryComponent))
	{
		return;
	}

	FInventorySlots NewViewSlots;
	TMap<int32, FString> AllowedTypeMap;
	AllowedTypeMap.Add(ViewSlotTypeID, TEXT("ViewedSlot"));
	NewViewSlots.InitializeInventory(ViewSlotCount, AllowedTypeMap);

	FInventorySlotsGroup& MasterGroup = ParentInventoryComponent->GetInventorySlotsGroup();

	MasterGroup.AddInventoryGroup(NewViewSlots);
}

void UViewedInventoryModule::OnModuleRemoved_Implementation()
{
	ClearAllEquipmentMeshes();
	Super::OnModuleRemoved_Implementation();
}

void UViewedInventoryModule::OnItemAdded_Implementation(UItemBase* Item, int32 GroupIndex, int32 SlotIndex)
{
	Super::OnItemAdded_Implementation(Item, GroupIndex, SlotIndex);

	if (!Item || GroupIndex != ViewSlotTypeID)
	{
		return;
	}

	UWearableModule* Wearable = Item->GetModule<UWearableModule>();
	if (!Wearable)
	{
		return;
	}

	FEquipmentMeshInfo MeshInfo;
	MeshInfo.SkeletalMesh = Wearable->ItemMesh.LoadSynchronous();
	MeshInfo.SocketName = Wearable->AttachSocketName;
	MeshInfo.RelativeTransform = FTransform(Wearable->MeshRotation, Wearable->MeshOffset);

	AttachEquipmentMesh(Wearable->EquipSlot, MeshInfo);
}

bool UViewedInventoryModule::AttachEquipmentMesh_Implementation(EEquipSlot EquipSlot,
                                                                const FEquipmentMeshInfo& MeshInfo)
{
	bool bDidAttachSuccessfully = false;

	if (GetFirstPersonMesh())
	{
		USkeletalMeshComponent* FPComp = nullptr;

		if (TObjectPtr<USkeletalMeshComponent>* FoundComp = EquipmentMeshesFirstPerson.Find(EquipSlot))
		{
			FPComp = *FoundComp;
		}

		if (MeshInfo.SkeletalMesh)
		{
			if (!FPComp)
			{
				FName UniqueCompName = MakeUniqueObjectName(GetOwningInventory()->GetOwner(),
				                                            USkeletalMeshComponent::StaticClass(),
				                                            FName(*FString::Printf(
					                                            TEXT("FP_Equip_%d"), static_cast<int32>(EquipSlot))));
				FPComp = NewObject<USkeletalMeshComponent>(GetOwningInventory()->GetOwner(), UniqueCompName);
				FPComp->SetOnlyOwnerSee(true);
				FPComp->bCastDynamicShadow = false;
				FPComp->CastShadow = false;
				EquipmentMeshesFirstPerson.Add(EquipSlot, FPComp);
			}

			FPComp->SetSkeletalMesh(MeshInfo.SkeletalMesh);
			FPComp->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
			FPComp->RegisterComponent();
			const FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
			FPComp->AttachToComponent(GetFirstPersonMesh(), AttachmentRules, MeshInfo.SocketName);
			FPComp->SetRelativeTransform(MeshInfo.RelativeTransform);
			FPComp->SetVisibility(true);

			bDidAttachSuccessfully = true;
		}
		else if (FPComp)
		{
			FPComp->SetSkeletalMesh(nullptr);
			FPComp->SetVisibility(false);
		}
	}

	if (GetMesh())
	{
		USkeletalMeshComponent* TPComp = nullptr;

		if (TObjectPtr<USkeletalMeshComponent>* FoundComp = EquipmentMeshes.Find(EquipSlot))
		{
			TPComp = *FoundComp;
		}

		if (MeshInfo.SkeletalMesh)
		{
			if (!TPComp)
			{
				FName UniqueCompName = MakeUniqueObjectName(GetOwningInventory()->GetOwner(),
				                                            USkeletalMeshComponent::StaticClass(),
				                                            FName(*FString::Printf(
					                                            TEXT("TP_Equip_%d"), static_cast<int32>(EquipSlot))));
				TPComp = NewObject<USkeletalMeshComponent>(GetOwningInventory()->GetOwner(), UniqueCompName);
				TPComp->SetOwnerNoSee(true);
				TPComp->RegisterComponent();

				EquipmentMeshes.Add(EquipSlot, TPComp);
			}

			TPComp->SetSkeletalMesh(MeshInfo.SkeletalMesh);
			const FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
			TPComp->AttachToComponent(GetMesh(), AttachmentRules, MeshInfo.SocketName);
			TPComp->SetRelativeTransform(MeshInfo.RelativeTransform);
			TPComp->SetVisibility(true);

			bDidAttachSuccessfully = true;
		}
		else if (TPComp)
		{
			TPComp->SetSkeletalMesh(nullptr);
			TPComp->SetVisibility(false);
		}
	}

	return bDidAttachSuccessfully;
}


void UViewedInventoryModule::DetachEquipmentMesh_Implementation(EEquipSlot EquipSlot)
{
	auto CleanupMesh = [](TMap<EEquipSlot, TObjectPtr<USkeletalMeshComponent>>& Map, EEquipSlot Slot)
	{
		if (TObjectPtr<USkeletalMeshComponent>* FoundPtr = Map.Find(Slot))
		{
			if (*FoundPtr)
			{
				(*FoundPtr)->DestroyComponent();
			}
			Map.Remove(Slot);
		}
	};

	CleanupMesh(EquipmentMeshes, EquipSlot);
	CleanupMesh(EquipmentMeshesFirstPerson, EquipSlot);
}

void UViewedInventoryModule::ClearAllEquipmentMeshes()
{
	for (auto& Pair : EquipmentMeshes)
	{
		if (Pair.Value) Pair.Value->DestroyComponent();
	}

	for (auto& Pair : EquipmentMeshesFirstPerson)
	{
		if (Pair.Value) Pair.Value->DestroyComponent();
	}

	EquipmentMeshes.Empty();
	EquipmentMeshesFirstPerson.Empty();
}

USkeletalMeshComponent* UViewedInventoryModule::GetMesh() const
{
	if (!GetOwningInventory() || !GetOwningInventory()->GetOwner())
	{
		return nullptr;
	}

	if (ACharacter* Character = Cast<ACharacter>(GetOwningInventory()->GetOwner()))
	{
		return Character->GetMesh();
	}
	return nullptr;
}

USkeletalMeshComponent* UViewedInventoryModule::GetFirstPersonMesh() const
{
	if (!GetOwningInventory() || !GetOwningInventory()->GetOwner())
	{
		return nullptr;
	}

	AActor* Owner = GetOwningInventory()->GetOwner();
	TArray<USkeletalMeshComponent*> Components;
	Owner->GetComponents(Components);

	for (USkeletalMeshComponent* Comp : Components)
	{
		if (Comp && Comp->GetName().Contains(TEXT("First Person Mesh")))
		{
			return Comp;
		}
	}
	return nullptr;
}
