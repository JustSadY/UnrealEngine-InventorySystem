#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InputHandler.generated.h"

UINTERFACE(Blueprintable)
class INVENTORYSYSTEM_API UInputHandler : public UInterface
{
	GENERATED_BODY()
};

class INVENTORYSYSTEM_API IInputHandler
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Input")
	void OnLeftMouseDown();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Input")
	void OnLeftMouseReleased();
};
