#include "InventoryComponent.h"
#include "InventorySystem.h"
#include "InventoryDebugSubsystem.h"
#include "Modules/InventoryModuleBase.h"
/**
 * Attaches a pre-instantiated module to the inventory system.
 * @param Module The module instance to install.
 * @return True if the module was successfully added and initialized.
 */
FInventoryOperationResult UInventoryComponent::InstallModule(UInventoryModuleBase* Module)
{
	double StartTime = FPlatformTime::Seconds();

	if (!IsValid(Module))
	{
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(TEXT("Invalid module"));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_InstallModule, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0), TEXT("Invalid module"));
		return FailResult;
	}

	if (InstalledModules.Contains(Module))
	{
		UE_LOG(LogInventory, Warning, TEXT("InstallModule: Module %s is already installed."), *Module->GetName());
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(FString::Printf(TEXT("Module %s is already installed"), *Module->GetName()));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_InstallModule, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0),
			FString::Printf(TEXT("Already installed: %s"), *Module->GetName()));
		return FailResult;
	}

	InstalledModules.Add(Module);
	Module->InitializeModule();

	if (HasBegunPlay())
	{
		Module->BeginPlay();
	}

	Module->OnModuleInstalled(this);

	// Enable tick if this module needs it
	if (Module->GetWantsTick())
	{
		SetComponentTickEnabled(true);
	}

	FInventoryOperationResult OkResult = FInventoryOperationResult::Ok();
	TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_InstallModule, OkResult,
		static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0),
		FString::Printf(TEXT("Module: %s"), *Module->GetName()));
	return OkResult;
}

/**
 * Creates and installs a module based on the provided class type.
 * @param ModuleClass The class of the module to instantiate.
 * @return True if the module was created and installed.
 */
FInventoryOperationResult UInventoryComponent::InstallModuleByClass(TSubclassOf<UInventoryModuleBase> ModuleClass)
{
	if (!ModuleClass)
	{
		return FInventoryOperationResult::Fail(TEXT("Invalid module class"));
	}

	for (UInventoryModuleBase* ExistingMod : InstalledModules)
	{
		if (IsValid(ExistingMod) && ExistingMod->IsA(ModuleClass))
		{
			UE_LOG(LogInventory, Warning, TEXT("InstallModuleByClass: Module class %s is already present."),
			       *ModuleClass->GetName());
			return FInventoryOperationResult::Fail(FString::Printf(TEXT("Module class %s is already present"), *ModuleClass->GetName()));
		}
	}

	UInventoryModuleBase* NewModule = NewObject<UInventoryModuleBase>(this, ModuleClass);
	if (!NewModule)
	{
		return FInventoryOperationResult::Fail(TEXT("Failed to create module instance"));
	}

	return InstallModule(NewModule);
}

/**
 * Detaches and cleans up a module from the inventory.
 * @param Module The module instance to remove.
 * @return True if the module was found and removed.
 */
FInventoryOperationResult UInventoryComponent::RemoveModule(UInventoryModuleBase* Module)
{
	double StartTime = FPlatformTime::Seconds();

	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogInventory, Warning, TEXT("RemoveModule: No authority or no owner"));
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(TEXT("No authority or no owner"));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_RemoveModule, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0), TEXT("No authority"));
		return FailResult;
	}

	if (!IsValid(Module))
	{
		FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(TEXT("Invalid module"));
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_RemoveModule, FailResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0), TEXT("Invalid module"));
		return FailResult;
	}

	if (InstalledModules.Remove(Module) > 0)
	{
		Module->OnModuleRemoved();

		// Disable tick if no remaining module needs it
		bool bAnyModuleWantsTick = false;
		for (UInventoryModuleBase* Mod : InstalledModules)
		{
			if (IsValid(Mod) && Mod->GetWantsTick())
			{
				bAnyModuleWantsTick = true;
				break;
			}
		}
		if (!bAnyModuleWantsTick)
		{
			SetComponentTickEnabled(false);
		}

		FInventoryOperationResult OkResult = FInventoryOperationResult::Ok();
		TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_RemoveModule, OkResult,
			static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0),
			FString::Printf(TEXT("Module: %s"), *Module->GetName()));
		return OkResult;
	}

	FInventoryOperationResult FailResult = FInventoryOperationResult::Fail(TEXT("Module not found in installed modules"));
	TrackInventoryOperation(GetWorld(), EInventoryOperationType::IOT_RemoveModule, FailResult,
		static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0), TEXT("Module not found"));
	return FailResult;
}
