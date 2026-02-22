#include "Modules/PassiveItemModule.h"

#include "Items/ItemBase.h"

UPassiveItemModule::UPassiveItemModule()
{
	bIsModuleActive = true;
}

void UPassiveItemModule::OnItemAddedToInventory_Implementation(AActor* Owner)
{
	Super::OnItemAddedToInventory_Implementation(Owner);
	Internal_ApplyEffects(EPassiveActivationType::OnInventory, Owner);
	Internal_ApplyEffects(EPassiveActivationType::Always, Owner);
}

void UPassiveItemModule::OnItemRemovedFromInventory_Implementation()
{
	UItemBase* ItemBase = GetOwnerItem();
	if (!ItemBase) return;
	if (AActor* OwnerActor = ItemBase->GetOwner())
		Internal_RemoveAllEffects(OwnerActor);
	Super::OnItemRemovedFromInventory_Implementation();
}


void UPassiveItemModule::Internal_ApplyEffects(EPassiveActivationType Condition, AActor* Target)
{
	if (!Target) return;

	for (const FPassiveEffectData& Data : PassiveEffectsToApply)
	{
		if (Data.ActivationType == Condition && Data.EffectClass)
		{
			if (UPassiveEffect* NewEffect = NewObject<UPassiveEffect>(Target, Data.EffectClass))
			{
				NewEffect->ApplyEffect(Target);
				ActiveEffectInstances.Add(NewEffect);
			}
		}
	}
}

void UPassiveItemModule::Internal_RemoveAllEffects(AActor* Target)
{
	if (!Target) return;

	for (UPassiveEffect* Effect : ActiveEffectInstances)
	{
		if (Effect)
		{
			Effect->RemoveEffect(Target);
		}
	}
	ActiveEffectInstances.Empty();
}
