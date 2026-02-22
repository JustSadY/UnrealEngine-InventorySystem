#include "InventoryDragDropValidation.h"
#include "InventorySystem.h"
#include "InventoryComponent.h"

TArray<FDragDropValidator> UInventoryDragDropValidation::CustomValidators;

FDragDropValidationResult UInventoryDragDropValidation::ValidateDragDrop(const FDragDropContext& Context)
{
	if (!Context.DraggedItem)
	{
		return FDragDropValidationResult::Invalid(TEXT("No item being dragged"));
	}

	if (!Context.SourceInventory)
	{
		return FDragDropValidationResult::Invalid(TEXT("Invalid source inventory"));
	}

	FDragDropContext ModifiedContext = Context;
	if (ModifiedContext.OperationType == EDragDropOperationType::DDOT_Move)
	{
		ModifiedContext.OperationType = DetermineOperationType(Context);
	}

	FDragDropValidationResult CustomResult = RunCustomValidators(ModifiedContext);
	if (!CustomResult.bIsValid)
	{
		return CustomResult;
	}

	switch (ModifiedContext.OperationType)
	{
	case EDragDropOperationType::DDOT_Move:
		return ValidateMove(ModifiedContext);

	case EDragDropOperationType::DDOT_Swap:
		return ValidateSwap(ModifiedContext);

	case EDragDropOperationType::DDOT_Stack:
		return ValidateStack(ModifiedContext);

	case EDragDropOperationType::DDOT_Split:
		return ValidateSplit(ModifiedContext);

	case EDragDropOperationType::DDOT_Transfer:
		return ValidateTransfer(ModifiedContext);

	default:
		return FDragDropValidationResult::Invalid(TEXT("Unknown operation type"));
	}
}

FDragDropValidationResult UInventoryDragDropValidation::ValidateMove(const FDragDropContext& Context)
{
	if (!IsTargetSlotCompatible(Context))
	{
		return FDragDropValidationResult::Invalid(TEXT("Target slot is not compatible with this item"));
	}

	const FInventorySlot* TargetSlot = GetTargetSlot(Context);
	if (!TargetSlot)
	{
		return FDragDropValidationResult::Invalid(TEXT("Invalid target slot"));
	}

	if (!TargetSlot->IsEmpty())
	{
		return FDragDropValidationResult::ValidWithWarning(
			EDragDropOperationType::DDOT_Swap,
			TEXT("Target slot is occupied - will swap items")
		);
	}

	return FDragDropValidationResult::Valid(EDragDropOperationType::DDOT_Move);
}

FDragDropValidationResult UInventoryDragDropValidation::ValidateSwap(const FDragDropContext& Context)
{
	const FInventorySlot* TargetSlot = GetTargetSlot(Context);
	if (!TargetSlot || TargetSlot->IsEmpty())
	{
		return FDragDropValidationResult::Invalid(TEXT("Cannot swap with empty slot"));
	}

	UItemBase* TargetItem = TargetSlot->GetItem();
	if (!TargetItem)
	{
		return FDragDropValidationResult::Invalid(TEXT("Invalid target item"));
	}

	if (!IsTargetSlotCompatible(Context))
	{
		return FDragDropValidationResult::Invalid(TEXT("Items cannot be swapped - incompatible slot types"));
	}

	return FDragDropValidationResult::Valid(EDragDropOperationType::DDOT_Swap);
}

FDragDropValidationResult UInventoryDragDropValidation::ValidateStack(const FDragDropContext& Context)
{
	const FInventorySlot* TargetSlot = GetTargetSlot(Context);
	if (!TargetSlot || TargetSlot->IsEmpty())
	{
		return FDragDropValidationResult::Invalid(TEXT("Cannot stack with empty slot"));
	}

	UItemBase* TargetItem = TargetSlot->GetItem();
	if (!CanItemsStack(Context.DraggedItem, TargetItem))
	{
		return FDragDropValidationResult::Invalid(TEXT("Items cannot be stacked together"));
	}

	if (TargetSlot->IsFull())
	{
		return FDragDropValidationResult::Invalid(TEXT("Target stack is already full"));
	}

	int32 AvailableSpace = TargetSlot->GetAvailableSpace();
	int32 DraggedAmount = Context.DraggedItem->GetCurrentStackSize();

	if (DraggedAmount > AvailableSpace)
	{
		return FDragDropValidationResult::ValidWithWarning(
			EDragDropOperationType::DDOT_Stack,
			FString::Printf(
				TEXT("Only %d items will stack, %d will remain"), AvailableSpace, DraggedAmount - AvailableSpace)
		);
	}

	return FDragDropValidationResult::Valid(EDragDropOperationType::DDOT_Stack);
}

FDragDropValidationResult UInventoryDragDropValidation::ValidateSplit(const FDragDropContext& Context)
{
	if (!Context.DraggedItem || !Context.DraggedItem->IsStackable())
	{
		return FDragDropValidationResult::Invalid(TEXT("Item is not stackable"));
	}

	if (Context.DraggedItem->GetCurrentStackSize() <= 1)
	{
		return FDragDropValidationResult::Invalid(TEXT("Cannot split a stack of 1"));
	}

	if (Context.SplitAmount <= 0 || Context.SplitAmount >= Context.DraggedItem->GetCurrentStackSize())
	{
		return FDragDropValidationResult::Invalid(TEXT("Invalid split amount"));
	}

	const FInventorySlot* TargetSlot = GetTargetSlot(Context);
	if (!TargetSlot)
	{
		return FDragDropValidationResult::Invalid(TEXT("Invalid target slot"));
	}

	if (!TargetSlot->IsEmpty())
	{
		return FDragDropValidationResult::Invalid(TEXT("Cannot split to an occupied slot"));
	}

	return FDragDropValidationResult::Valid(EDragDropOperationType::DDOT_Split);
}

FDragDropValidationResult UInventoryDragDropValidation::ValidateTransfer(const FDragDropContext& Context)
{
	if (!Context.TargetInventory)
	{
		return FDragDropValidationResult::Invalid(TEXT("Invalid target inventory"));
	}

	if (Context.SourceInventory == Context.TargetInventory)
	{
		return ValidateMove(Context);
	}

	if (!Context.TargetInventory->CanAddItem(Context.DraggedItem, Context.TargetGroupIndex))
	{
		return FDragDropValidationResult::Invalid(TEXT("Target inventory cannot accept this item"));
	}

	return FDragDropValidationResult::Valid(EDragDropOperationType::DDOT_Transfer);
}

EDragDropOperationType UInventoryDragDropValidation::DetermineOperationType(const FDragDropContext& Context)
{
	if (Context.bIsShiftHeld && Context.DraggedItem && Context.DraggedItem->IsStackable())
	{
		return EDragDropOperationType::DDOT_Split;
	}

	if (Context.SourceInventory != Context.TargetInventory && Context.TargetInventory)
	{
		return EDragDropOperationType::DDOT_Transfer;
	}

	const FInventorySlot* TargetSlot = GetTargetSlot(Context);
	if (!TargetSlot)
	{
		return EDragDropOperationType::DDOT_Move;
	}

	if (TargetSlot->IsEmpty())
	{
		return EDragDropOperationType::DDOT_Move;
	}

	if (CanItemsStack(Context.DraggedItem, TargetSlot->GetItem()))
	{
		return EDragDropOperationType::DDOT_Stack;
	}

	return EDragDropOperationType::DDOT_Swap;
}

bool UInventoryDragDropValidation::ExecuteDragDrop(const FDragDropContext& Context)
{
	FDragDropValidationResult ValidationResult = ValidateDragDrop(Context);

	if (!ValidationResult.bIsValid)
	{
		UE_LOG(LogInventory, Warning, TEXT("ExecuteDragDrop: Validation failed - %s"), *ValidationResult.ErrorMessage);
		return false;
	}

	switch (ValidationResult.SuggestedOperation)
	{
	case EDragDropOperationType::DDOT_Move:
		return Context.SourceInventory->TransferItem(
			Context.SourceGroupIndex, Context.SourceSlotIndex,
			Context.TargetGroupIndex, Context.TargetSlotIndex
		).bSuccess;

	case EDragDropOperationType::DDOT_Swap:
		return Context.SourceInventory->TransferItem(
			Context.SourceGroupIndex, Context.SourceSlotIndex,
			Context.TargetGroupIndex, Context.TargetSlotIndex
		).bSuccess;

	case EDragDropOperationType::DDOT_Stack:
		return Context.SourceInventory->TryStackItem(Context.DraggedItem, Context.TargetGroupIndex).bSuccess;

	case EDragDropOperationType::DDOT_Split:
		UE_LOG(LogInventory, Warning, TEXT("Split operation not yet implemented"));
		return false;

	case EDragDropOperationType::DDOT_Transfer:
		if (Context.TargetInventory)
		{
			if (Context.SourceInventory->RemoveItem(Context.DraggedItem).bSuccess)
			{
				if (!Context.TargetInventory->AddItem(Context.DraggedItem, Context.TargetGroupIndex).bSuccess)
				{
					Context.SourceInventory->AddItem(Context.DraggedItem);
					UE_LOG(LogInventory, Warning, TEXT("ExecuteDragDrop: Transfer failed, item returned to source"));
					return false;
				}
				return true;
			}
		}
		return false;

	default:
		return false;
	}
}

void UInventoryDragDropValidation::AddCustomValidator(FDragDropValidator Validator)
{
	if (Validator.IsBound())
	{
		CustomValidators.Add(Validator);
	}
}

void UInventoryDragDropValidation::ClearCustomValidators()
{
	CustomValidators.Empty();
}

bool UInventoryDragDropValidation::IsTargetSlotCompatible(const FDragDropContext& Context)
{
	if (!Context.TargetInventory || !Context.DraggedItem)
	{
		return false;
	}

	FInventorySlots* TargetGroup = Context.TargetInventory->GetInventorySlotsGroup().GetGroupByID(
		Context.TargetGroupIndex);
	if (!TargetGroup)
	{
		return false;
	}

	return TargetGroup->IsTypeSupported(Context.DraggedItem);
}

bool UInventoryDragDropValidation::CanItemsStack(UItemBase* ItemA, UItemBase* ItemB)
{
	if (!ItemA || !ItemB)
	{
		return false;
	}

	if (!ItemA->IsStackable() || !ItemB->IsStackable())
	{
		return false;
	}

	return ItemA->GetItemDefinition().GetItemID() == ItemB->GetItemDefinition().GetItemID();
}

const FInventorySlot* UInventoryDragDropValidation::GetTargetSlot(const FDragDropContext& Context)
{
	if (!Context.TargetInventory)
	{
		return nullptr;
	}

	FInventorySlots* TargetGroup = Context.TargetInventory->GetInventorySlotsGroup().GetGroupByID(
		Context.TargetGroupIndex);
	if (!TargetGroup)
	{
		return nullptr;
	}

	return TargetGroup->GetSlotAtIndex(Context.TargetSlotIndex);
}


FDragDropValidationResult UInventoryDragDropValidation::RunCustomValidators(const FDragDropContext& Context)
{
	for (const FDragDropValidator& Validator : CustomValidators)
	{
		if (Validator.IsBound())
		{
			FDragDropValidationResult Result = Validator.Execute(Context);
			if (!Result.bIsValid)
			{
				return Result;
			}
		}
	}

	return FDragDropValidationResult::Valid(Context.OperationType);
}

UTexture2D* UDragDropVisualFeedback::GetCursorIconForOperation(EDragDropOperationType OperationType)
{
	return nullptr;
}

FLinearColor UDragDropVisualFeedback::GetValidationColor(bool bIsValid, bool bHasWarning)
{
	if (!bIsValid)
	{
		return FLinearColor::Red;
	}

	if (bHasWarning)
	{
		return FLinearColor::Yellow;
	}

	return FLinearColor::Green;
}

FText UDragDropVisualFeedback::GetOperationTooltip(EDragDropOperationType OperationType, bool bIsValid,
                                                   const FString& Message)
{
	FString BaseText;

	switch (OperationType)
	{
	case EDragDropOperationType::DDOT_Move:
		BaseText = TEXT("Move Item");
		break;
	case EDragDropOperationType::DDOT_Swap:
		BaseText = TEXT("Swap Items");
		break;
	case EDragDropOperationType::DDOT_Stack:
		BaseText = TEXT("Stack Items");
		break;
	case EDragDropOperationType::DDOT_Split:
		BaseText = TEXT("Split Stack");
		break;
	case EDragDropOperationType::DDOT_Transfer:
		BaseText = TEXT("Transfer Item");
		break;
	default:
		BaseText = TEXT("Unknown");
		break;
	}

	if (!Message.IsEmpty())
	{
		BaseText += FString::Printf(TEXT("\n%s"), *Message);
	}

	return FText::FromString(BaseText);
}
