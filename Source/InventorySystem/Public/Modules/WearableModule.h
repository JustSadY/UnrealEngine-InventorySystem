#pragma once

#include "CoreMinimal.h"
#include "Modules/ItemModuleBase.h"
#include "Types/EquipSlot.h"
#include "WearableModule.generated.h"

UCLASS(Blueprintable, meta = (DisplayName = "Wearable Module"))
class INVENTORYSYSTEM_API UWearableModule : public UItemModuleBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wearable|Visual")
	EEquipSlot EquipSlot = EEquipSlot::EES_MAX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wearable|Visual")
	TSoftObjectPtr<USkeletalMesh> ItemMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wearable|Visual")
	FName AttachSocketName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wearable|Visual")
	FVector MeshOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wearable|Visual")
	FRotator MeshRotation = FRotator::ZeroRotator;
};
