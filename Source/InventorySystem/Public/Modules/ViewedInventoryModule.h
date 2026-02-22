#pragma once

#include "CoreMinimal.h"
#include "Modules/InventoryModuleBase.h"
#include "Types/EquipSlot.h"
#include "ViewedInventoryModule.generated.h"

USTRUCT(BlueprintType)
struct FEquipmentMeshInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	TObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FName SocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FTransform RelativeTransform;

	FEquipmentMeshInfo()
		: SkeletalMesh(nullptr)
		  , SocketName(NAME_None)
		  , RelativeTransform(FTransform::Identity)
	{
	}
};
UCLASS(BlueprintType, Blueprintable)
class INVENTORYSYSTEM_API UViewedInventoryModule : public UInventoryModuleBase
{
	GENERATED_BODY()

public:
	UViewedInventoryModule();

	virtual void OnModuleInstalled_Implementation(UInventoryComponent* ParentInventoryComponent) override;
	virtual void OnModuleRemoved_Implementation() override;
	virtual void OnItemAdded_Implementation(UItemBase* Item, int32 GroupIndex, int32 SlotIndex) override;
	
	UFUNCTION(BlueprintNativeEvent, Category = "Visuals")
	bool AttachEquipmentMesh(EEquipSlot EquipSlot, const FEquipmentMeshInfo& MeshInfo);
	
	UFUNCTION(BlueprintNativeEvent, Category = "Visuals")
	void DetachEquipmentMesh(EEquipSlot EquipSlot);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration")
	int32 ViewSlotCount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration")
	int32 ViewSlotTypeID;

	UPROPERTY()
	TMap<EEquipSlot, TObjectPtr<USkeletalMeshComponent>> EquipmentMeshes;

	UPROPERTY()
	TMap<EEquipSlot, TObjectPtr<USkeletalMeshComponent>> EquipmentMeshesFirstPerson;

private:
	void ClearAllEquipmentMeshes();

	USkeletalMeshComponent* GetMesh() const;
	USkeletalMeshComponent* GetFirstPersonMesh() const;
};
