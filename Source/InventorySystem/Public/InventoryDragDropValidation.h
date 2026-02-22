#pragma once

#include "CoreMinimal.h"
#include "Items/ItemBase.h"
#include "Struct/InventorySlot.h"
#include "InventoryDragDropValidation.generated.h"

class UInventoryComponent;

UENUM(BlueprintType)
enum class EDragDropOperationType : uint8
{
	DDOT_Move UMETA(DisplayName = "Move"),
	DDOT_Swap UMETA(DisplayName = "Swap"),
	DDOT_Stack UMETA(DisplayName = "Stack"),
	DDOT_Split UMETA(DisplayName = "Split"),
	DDOT_Transfer UMETA(DisplayName = "Transfer")
};

USTRUCT(BlueprintType)
struct FDragDropContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Drag Drop")
	UItemBase* DraggedItem = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Drag Drop")
	int32 SourceGroupIndex = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Drag Drop")
	int32 SourceSlotIndex = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Drag Drop")
	int32 TargetGroupIndex = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Drag Drop")
	int32 TargetSlotIndex = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Drag Drop")
	EDragDropOperationType OperationType = EDragDropOperationType::DDOT_Move;

	UPROPERTY(BlueprintReadOnly, Category = "Drag Drop")
	int32 SplitAmount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Drag Drop")
	UInventoryComponent* SourceInventory = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Drag Drop")
	UInventoryComponent* TargetInventory = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Drag Drop")
	bool bIsCtrlHeld = false;

	UPROPERTY(BlueprintReadOnly, Category = "Drag Drop")
	bool bIsShiftHeld = false;

	UPROPERTY(BlueprintReadOnly, Category = "Drag Drop")
	bool bIsAltHeld = false;
};

USTRUCT(BlueprintType)
struct FDragDropValidationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Validation")
	bool bIsValid = false;

	UPROPERTY(BlueprintReadOnly, Category = "Validation")
	FString ErrorMessage;

	UPROPERTY(BlueprintReadOnly, Category = "Validation")
	EDragDropOperationType SuggestedOperation = EDragDropOperationType::DDOT_Move;

	UPROPERTY(BlueprintReadOnly, Category = "Validation")
	bool bShowWarning = false;

	UPROPERTY(BlueprintReadOnly, Category = "Validation")
	FString WarningMessage;

	static FDragDropValidationResult Valid(EDragDropOperationType Operation)
	{
		FDragDropValidationResult Result;
		Result.bIsValid = true;
		Result.SuggestedOperation = Operation;
		return Result;
	}

	static FDragDropValidationResult Invalid(const FString& Reason)
	{
		FDragDropValidationResult Result;
		Result.bIsValid = false;
		Result.ErrorMessage = Reason;
		return Result;
	}

	static FDragDropValidationResult ValidWithWarning(EDragDropOperationType Operation, const FString& Warning)
	{
		FDragDropValidationResult Result;
		Result.bIsValid = true;
		Result.SuggestedOperation = Operation;
		Result.bShowWarning = true;
		Result.WarningMessage = Warning;
		return Result;
	}
};

DECLARE_DELEGATE_RetVal_OneParam(FDragDropValidationResult, FDragDropValidator, const FDragDropContext&);

/**
 * Validates and executes inventory drag-and-drop operations.
 * Supports custom validators via AddCustomValidator for game-specific rules.
 */
UCLASS(BlueprintType)
class INVENTORYSYSTEM_API UInventoryDragDropValidation : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory|Drag Drop")
	static FDragDropValidationResult ValidateDragDrop(const FDragDropContext& Context);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Drag Drop")
	static FDragDropValidationResult ValidateMove(const FDragDropContext& Context);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Drag Drop")
	static FDragDropValidationResult ValidateSwap(const FDragDropContext& Context);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Drag Drop")
	static FDragDropValidationResult ValidateStack(const FDragDropContext& Context);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Drag Drop")
	static FDragDropValidationResult ValidateSplit(const FDragDropContext& Context);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Drag Drop")
	static FDragDropValidationResult ValidateTransfer(const FDragDropContext& Context);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Drag Drop")
	static EDragDropOperationType DetermineOperationType(const FDragDropContext& Context);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Drag Drop")
	static bool ExecuteDragDrop(const FDragDropContext& Context);

	static void AddCustomValidator(FDragDropValidator Validator);
	static void ClearCustomValidators();

protected:
	static bool IsTargetSlotCompatible(const FDragDropContext& Context);
	static bool CanItemsStack(UItemBase* ItemA, UItemBase* ItemB);
	static const FInventorySlot* GetTargetSlot(const FDragDropContext& Context);
	static FDragDropValidationResult RunCustomValidators(const FDragDropContext& Context);

	static TArray<FDragDropValidator> CustomValidators;
};

UCLASS(BlueprintType)
class INVENTORYSYSTEM_API UDragDropVisualFeedback : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory|Drag Drop|Visuals")
	static UTexture2D* GetCursorIconForOperation(EDragDropOperationType OperationType);

	UFUNCTION(BlueprintPure, Category = "Inventory|Drag Drop|Visuals")
	static FLinearColor GetValidationColor(bool bIsValid, bool bHasWarning);

	UFUNCTION(BlueprintPure, Category = "Inventory|Drag Drop|Visuals")
	static FText GetOperationTooltip(EDragDropOperationType OperationType, bool bIsValid, const FString& Message);
};
