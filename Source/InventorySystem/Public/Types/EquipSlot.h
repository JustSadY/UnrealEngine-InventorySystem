#pragma once

#include "CoreMinimal.h"
#include "EquipSlot.generated.h"

UENUM(BlueprintType)
enum class EEquipSlot : uint8
{
	EES_Head        UMETA(DisplayName = "Head"),
	EES_Chest       UMETA(DisplayName = "Chest"), 
	EES_Hands       UMETA(DisplayName = "Hands"),
	EES_Legs        UMETA(DisplayName = "Legs"),
	EES_Feet        UMETA(DisplayName = "Feet"),
	EES_MainHand    UMETA(DisplayName = "Main Hand"),
	EES_OffHand     UMETA(DisplayName = "Off Hand"),
	EES_Back        UMETA(DisplayName = "Back"),
	EES_Accessory1  UMETA(DisplayName = "Accessory 1"),
	EES_Accessory2  UMETA(DisplayName = "Accessory 2"),

	EES_MAX         UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct FEquipSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector RelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator RelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector RelativeScale = FVector(1.0f);
};
