#pragma once

#include "Modules/ModuleManager.h"
#include "HAL/IConsoleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogInventory, Log, All);

class FInventorySystemModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

#if !UE_BUILD_SHIPPING
private:
	TArray<IConsoleObject*> ConsoleCommands;

	void RegisterConsoleCommands();
	void UnregisterConsoleCommands();
#endif
};
