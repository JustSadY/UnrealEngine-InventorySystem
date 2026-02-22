// Copyright Epic Games, Inc. All Rights Reserved.

#include "InventorySystem.h"

#if !UE_BUILD_SHIPPING
#include "InventoryDebugSubsystem.h"
#include "PoolSystem/ItemPoolSubsystem.h"
#include "PoolSystem/EngineItemPoolSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#endif

DEFINE_LOG_CATEGORY(LogInventory);

#define LOCTEXT_NAMESPACE "FInventorySystemModule"

#if !UE_BUILD_SHIPPING
static UInventoryDebugSubsystem* GetDebugSubsystem()
{
	if (!GEngine) return nullptr;

	const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
	for (const FWorldContext& Context : WorldContexts)
	{
		if (Context.World() && (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE))
		{
			if (UGameInstance* GI = Context.World()->GetGameInstance())
			{
				if (UInventoryDebugSubsystem* DS = GI->GetSubsystem<UInventoryDebugSubsystem>())
				{
					return DS;
				}
			}
		}
	}
	return nullptr;
}

static UItemPoolSubsystem* GetPoolSubsystem()
{
	if (!GEngine) return nullptr;

	const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
	for (const FWorldContext& Context : WorldContexts)
	{
		if (Context.World() && (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE))
		{
			if (UItemPoolSubsystem* PS = Context.World()->GetSubsystem<UItemPoolSubsystem>())
			{
				return PS;
			}
		}
	}
	return nullptr;
}
#endif

void FInventorySystemModule::StartupModule()
{
#if !UE_BUILD_SHIPPING
	RegisterConsoleCommands();
#endif
}

void FInventorySystemModule::ShutdownModule()
{
#if !UE_BUILD_SHIPPING
	UnregisterConsoleCommands();
#endif
}

#if !UE_BUILD_SHIPPING
void FInventorySystemModule::RegisterConsoleCommands()
{
	// Inventory.Debug.Overlay <0|1> [Mode]
	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Inventory.Debug.Overlay"),
		TEXT("Toggle inventory debug overlay. Usage: Inventory.Debug.Overlay <0|1> [Mode: 0=None,1=Basic,2=Detailed,3=Performance,4=Network]"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			UInventoryDebugSubsystem* DS = GetDebugSubsystem();
			if (!DS)
			{
				UE_LOG(LogInventory, Warning, TEXT("No InventoryDebugSubsystem found"));
				return;
			}

			bool bEnabled = Args.Num() > 0 ? FCString::Atoi(*Args[0]) != 0 : true;
			EInventoryDebugMode Mode = EInventoryDebugMode::IDM_Basic;
			if (Args.Num() > 1)
			{
				int32 ModeVal = FCString::Atoi(*Args[1]);
				Mode = static_cast<EInventoryDebugMode>(FMath::Clamp(ModeVal, 0, 4));
			}
			DS->SetDebugOverlay(bEnabled, Mode);
		}),
		ECVF_Default
	));

	// Inventory.Debug.Tracking <0|1>
	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Inventory.Debug.Tracking"),
		TEXT("Enable/disable operation tracking. Usage: Inventory.Debug.Tracking <0|1>"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			UInventoryDebugSubsystem* DS = GetDebugSubsystem();
			if (!DS)
			{
				UE_LOG(LogInventory, Warning, TEXT("No InventoryDebugSubsystem found"));
				return;
			}

			bool bEnabled = Args.Num() > 0 ? FCString::Atoi(*Args[0]) != 0 : true;
			DS->SetOperationTracking(bEnabled);
		}),
		ECVF_Default
	));

	// Inventory.Debug.PerfThreshold <WarningMs> <CriticalMs>
	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Inventory.Debug.PerfThreshold"),
		TEXT("Set performance thresholds. Usage: Inventory.Debug.PerfThreshold <WarningMs> <CriticalMs>"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			UInventoryDebugSubsystem* DS = GetDebugSubsystem();
			if (!DS)
			{
				UE_LOG(LogInventory, Warning, TEXT("No InventoryDebugSubsystem found"));
				return;
			}

			if (Args.Num() < 2)
			{
				UE_LOG(LogInventory, Warning, TEXT("Usage: Inventory.Debug.PerfThreshold <WarningMs> <CriticalMs>"));
				return;
			}

			float WarningMs = FCString::Atof(*Args[0]);
			float CriticalMs = FCString::Atof(*Args[1]);
			DS->SetPerformanceThresholds(WarningMs, CriticalMs);
		}),
		ECVF_Default
	));

	// Inventory.Debug.PoolStats
	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Inventory.Debug.PoolStats"),
		TEXT("Print pool hit/miss statistics"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			UItemPoolSubsystem* PS = GetPoolSubsystem();
			if (PS)
			{
				FString Summary = PS->GetAllPoolStatsSummary();
				UE_LOG(LogInventory, Log, TEXT("\n%s"), *Summary);
			}
			else
			{
				UE_LOG(LogInventory, Warning, TEXT("No ItemPoolSubsystem found"));
			}

			if (UEngineItemPoolSubsystem* EPS = GEngine->GetEngineSubsystem<UEngineItemPoolSubsystem>())
			{
				FString EngineSummary = EPS->GetAllPoolStatsSummary();
				UE_LOG(LogInventory, Log, TEXT("\n%s"), *EngineSummary);
			}
		}),
		ECVF_Default
	));

	// Inventory.Debug.OpSummary
	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Inventory.Debug.OpSummary"),
		TEXT("Print operation tracking summary"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			UInventoryDebugSubsystem* DS = GetDebugSubsystem();
			if (DS)
			{
				FString Summary = DS->GetOperationSummary();
				UE_LOG(LogInventory, Log, TEXT("\n%s"), *Summary);
			}
			else
			{
				UE_LOG(LogInventory, Warning, TEXT("No InventoryDebugSubsystem found"));
			}
		}),
		ECVF_Default
	));

	// Inventory.Debug.ResetStats
	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Inventory.Debug.ResetStats"),
		TEXT("Reset all debug/tracking/pool statistics"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			UInventoryDebugSubsystem* DS = GetDebugSubsystem();
			if (DS)
			{
				DS->GetOperationTracker().Reset();
				UE_LOG(LogInventory, Log, TEXT("Operation tracker stats reset"));
			}

			UItemPoolSubsystem* PS = GetPoolSubsystem();
			if (PS)
			{
				PS->ResetPoolStats();
			}

			if (UEngineItemPoolSubsystem* EPS = GEngine->GetEngineSubsystem<UEngineItemPoolSubsystem>())
			{
				EPS->ResetPoolStats();
			}
		}),
		ECVF_Default
	));

	// Inventory.Debug.FrameTracking <0|1>
	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Inventory.Debug.FrameTracking"),
		TEXT("Enable/disable per-frame cost tracking. Usage: Inventory.Debug.FrameTracking <0|1>"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			UInventoryDebugSubsystem* DS = GetDebugSubsystem();
			if (!DS)
			{
				UE_LOG(LogInventory, Warning, TEXT("No InventoryDebugSubsystem found"));
				return;
			}

			bool bEnabled = Args.Num() > 0 ? FCString::Atoi(*Args[0]) != 0 : true;
			DS->SetFrameTracking(bEnabled);
		}),
		ECVF_Default
	));
}

void FInventorySystemModule::UnregisterConsoleCommands()
{
	for (IConsoleObject* Cmd : ConsoleCommands)
	{
		if (Cmd)
		{
			IConsoleManager::Get().UnregisterConsoleObject(Cmd);
		}
	}
	ConsoleCommands.Empty();
}
#endif

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FInventorySystemModule, InventorySystem)
