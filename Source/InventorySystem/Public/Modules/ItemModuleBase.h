#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemModuleBase.generated.h"

class UItemBase;

UCLASS(Blueprintable, Abstract, EditInlineNew, DefaultToInstanced)
class INVENTORYSYSTEM_API UItemModuleBase : public UObject
{
	GENERATED_BODY()

public:
	UItemModuleBase();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Module")
	TObjectPtr<UItemBase> OwnerItem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Module")
	bool bIsModuleActive;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Module")
	int32 Priority;

public:
	UFUNCTION(BlueprintCallable, Category = "Module")
	virtual void Initialize(UItemBase* InOwnerItem);

	UFUNCTION(BlueprintCallable, Category = "Module")
	virtual void Reset();

	UFUNCTION(BlueprintNativeEvent, Category = "Module|Events")
	void OnItemAddedToInventory(AActor* Owner);
	virtual void OnItemAddedToInventory_Implementation(AActor* Owner);

	UFUNCTION(BlueprintNativeEvent, Category = "Module|Events")
	void OnItemRemovedFromInventory();
	virtual void OnItemRemovedFromInventory_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "Module|Events")
	void OnItemMerged(UItemBase* OtherItem, bool bIsSource);
	virtual void OnItemMerged_Implementation(UItemBase* OtherItem, bool bIsSource);

	UFUNCTION(BlueprintNativeEvent, Category = "Module|Events")
	void OnItemSplit(UItemBase* NewItem, int32 Amount);
	virtual void OnItemSplit_Implementation(UItemBase* NewItem, int32 Amount);

	UFUNCTION(BlueprintNativeEvent, Category = "Module|Save")
	TMap<FString, FString> SerializeModuleData() const;
	virtual TMap<FString, FString> SerializeModuleData_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, Category = "Module|Save")
	void DeserializeModuleData(const TMap<FString, FString>& Data);
	virtual void DeserializeModuleData_Implementation(const TMap<FString, FString>& Data);

	UFUNCTION(BlueprintCallable, Category = "Module")
	virtual UItemModuleBase* DuplicateModule(UItemBase* TargetItem);

	UFUNCTION(BlueprintPure, Category = "Module")
	UItemBase* GetOwnerItem() const { return OwnerItem; }

	UFUNCTION(BlueprintPure, Category = "Module")
	bool IsModuleActive() const { return bIsModuleActive; }

	UFUNCTION(BlueprintPure, Category = "Module")
	int32 GetPriority() const { return Priority; }

	UFUNCTION(BlueprintCallable, Category = "Module")
	void SetModuleActive(bool bActive) { bIsModuleActive = bActive; }
};
