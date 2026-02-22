#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Equippable.generated.h"

UINTERFACE(Blueprintable)
class INVENTORYSYSTEM_API UEquippable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Equippable interface
 */
class INVENTORYSYSTEM_API IEquippable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Equipment")
	void OnEquip(AActor* OwningPawn);
	virtual void OnEquip_Implementation(AActor* OwningPawn){}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Equipment")
	void OnUnequip(AActor* OwningPawn);
	virtual void OnUnequip_Implementation(AActor* OwningPawn){}
};
