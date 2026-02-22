#pragma once

#include "CoreMinimal.h"
#include "ItemModuleBase.h"
#include "InventorySystem/Public/Items/PassiveEffect/PassiveEffect.h"
#include "PassiveItemModule.generated.h"

UENUM(BlueprintType)
enum class EPassiveActivationType : uint8
{
	None UMETA(DisplayName = "None"),
	OnInventory UMETA(DisplayName = "While in Inventory"),
	OnEquipped UMETA(DisplayName = "While Equipped"),
	Always UMETA(DisplayName = "Always Active")
};

USTRUCT(BlueprintType)
struct FPassiveEffectData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	TSubclassOf<UPassiveEffect> EffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	EPassiveActivationType ActivationType = EPassiveActivationType::None;
};

UCLASS(Blueprintable, EditInlineNew)
class INVENTORYSYSTEM_API UPassiveItemModule : public UItemModuleBase
{
	GENERATED_BODY()

public:
	UPassiveItemModule();

	virtual void OnItemAddedToInventory_Implementation(AActor* Owner) override;
	virtual void OnItemRemovedFromInventory_Implementation() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Passives")
	TArray<FPassiveEffectData> PassiveEffectsToApply;

	UPROPERTY()
	TArray<UPassiveEffect*> ActiveEffectInstances;

private:
	void Internal_ApplyEffects(EPassiveActivationType Condition, AActor* Target);
	void Internal_RemoveAllEffects(AActor* Target);
};
