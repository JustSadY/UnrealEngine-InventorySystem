#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Pawn.h"
#include "PassiveEffect.generated.h"

UCLASS(Blueprintable)
class INVENTORYSYSTEM_API UPassiveEffect : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
	FName EffectName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
	FText EffectDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
	float EffectValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
	FName AffectedAttribute;

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item|Events")
	void ApplyEffect(AActor* TargetPawn);
	virtual void ApplyEffect_Implementation(AActor* TargetPawn) {}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item|Events")
	void RemoveEffect(AActor* TargetPawn);
	virtual void RemoveEffect_Implementation(AActor* TargetPawn) {}
};
