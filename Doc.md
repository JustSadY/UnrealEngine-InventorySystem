# 📖 Modular Inventory System - Comprehensive User Manual

Welcome to the **Modular Inventory System**! This manual provides an in-depth, comprehensive look into the architecture, implementation, and advanced usage of the plugin. Whether you are a Designer utilizing Blueprints or a Programmer writing custom C++ modules, this guide covers everything you need to know.

---

## 📑 Table of Contents

1. [Architectural Overview](#1-architectural-overview)
2. [Setting Up the Core Systems](#2-setting-up-the-core-systems)
    * `UInventoryComponent` Initialization
    * Slot Groups and Capacity Management
3. [The Item System Demystified](#3-the-item-system-demystified)
    * `UItemBase` & `FItemDefinition`
    * Stacking and Quantities
4. [Mastering Modules (The Secret Sauce)](#4-mastering-modules-the-secret-sauce)
    * `UItemModuleBase` - Item Behaviors
    * `UInventoryModuleBase` - Inventory Behaviors
5. [C++ and Blueprint API Reference](#5-c-and-blueprint-api-reference)
    * Adding, Removing, and Transferring
    * Searching and Sorting
6. [Multiplayer & Networking Guide](#6-multiplayer--networking-guide)
7. [Advanced Customization: Writing Your Own Modules](#7-advanced-customization-writing-your-own-modules)

---

## 1. Architectural Overview

At its core, this plugin uses **Composition over Inheritance**.
Traditional inventory systems force you down deep, rigid inheritance paths (`Item -> Weapon -> MeleeWeapon -> Sword`). This plugin eliminates that.
Instead, an item is just a blank container (`UItemBase`). You add functionality by attaching **Modules** (`UItemModuleBase`) to that container. A "Sword" is simply an item with an *EquipModule* and a *DamageModule* attached.

### The Big Three:
1. **`UInventoryComponent`**: The warehouse. Holds arrays of slots and manages all transactions safely.
2. **`UItemBase`**: The actual physical (or conceptual) item existing in the inventory.
3. **`FItemDefinition`**: The data template defining the visual and logical hard-coded rules (Name, ID, Texture, Allowed Slots) for the item.

---

## 2. Setting Up the Core Systems

### Adding `UInventoryComponent` to an Actor
To give a Character, Chest, or Vendor an inventory:
1. Open your Actor Blueprint (e.g., `BP_PlayerCharacter`).
2. Add the **Inventory Component** (`UInventoryComponent`) to the component hierarchy.
3. Select the component and look at the **Details Panel** under the **Inventory** category.

### Configuring Slot Groups (`InventorySlotsGroup`)
Slot Groups define *where* an item is physically allowed to be placed. Unlike simple arrays, this system supports distinct sections (E.g., "Backpack" vs. "Hotbar").

1. In the `InventoryComponent` details, expand `InventorySlotsGroup`.
2. Expand `InventoryGroups` and add elements (e.g., Index 0, Index 1).
3. For **Index 0 (Main Backpack)**:
   - Expand `TypeIDMap`. Add a mapping (e.g., Integer `0` -> String `"Backpack"`). **0** is now the unique ID for the Backpack.
   - Expand `Slots`. Add as many elements as you want capacity (e.g., 20 elements = 20 Backpack slots).
4. For **Index 1 (Hotbar)**:
   - Set `TypeIDMap` to (Integer `1` -> String `"Hotbar"`).
   - Add 5 empty slots.

> **Why TypeIDs?** Later, when you create an item, you tell the item *which TypeIDs it is allowed to enter*. A health potion might be allowed in `0` and `1`, but a heavy armor piece might only be allowed in `0`.

---

## 3. The Item System Demystified

### Creating a New Item Definition
You don't need a new class for every item. You just need a new Blueprint based on `UItemBase`.

1. Create a Blueprint Class -> Inherit from `ItemBase` (e.g., `BP_Item_HealthPotion`).
2. Open it, and in the **Class Defaults**, configure the **Item Data (`FItemDefinition`)**:
   - `ItemID`: **Must be unique** (e.g., `"consumable_health_01"`). Used for global lookups and stacking checks.
   - `ItemName`: `"Minor Health Potion"`
   - `ItemDescription`: `"Restores 50 HP."`
   - `ItemIcon`: Select a `Texture2D`.
   - `Inventory Slot Type IDs`: Click `+` and add `0` and `1` (Allows it to go to Backpack and Hotbar).

### Stacking Rules
If you want the player to hold 10 potions in a single slot:
1. Check **Is Stackable** to `True`.
2. Set **Max Stack Size** to `10`.
The system will automatically handle overflow. If you add 15 potions, the system will fill one slot with 10, and put 5 in the next available slot.

---

## 4. Mastering Modules (The Secret Sauce)

Modules define **what an item does** or **how an inventory reacts**.

### Item Modules (`UItemModuleBase`)
To make the Health Potion actually do something, you attach a module to it.
1. Open `BP_Item_HealthPotion`.
2. Scroll to the **Item Modules** array in Class Defaults.
3. Add an element and choose your custom module (e.g., `ConsumableModule`).
4. Expand it to set its local variables (e.g., `HealAmount = 50`).

### Inventory Modules (`UInventoryModuleBase`)
Inventories themselves can have modules. For example, a `WeightLimitModule`.
1. Select your `InventoryComponent` in the Character.
2. In the Details panel, find **Installed Modules**.
3. Add a `WeightLimitModule`. The module can hook into `OnItemAdded` and `OnItemRemoved` events natively to recalculate total weight dynamically.

---

## 5. C++ and Blueprint API Reference

All heavy lifting is done in C++, exposed beautifully to Blueprints. Every transaction returns a robust struct: `FInventoryOperationResult`. It explicitly states if an action succeeded or failed, and *why*.

### Adding Items
```cpp
// You must have an instanced item first.
UItemBase* PotionInstance = InventoryComp->CreateItemInstance(PotionBPClass);

// Add to TypeID 0 (Backpack). Pass -1 to let the system auto-find the best valid group.
FInventoryOperationResult Result = InventoryComp->AddItem(PotionInstance, 0);

if (!Result.bSuccess)
{
    // Possible Messages: "Target inventory group not found," "Compatible groups are full," etc.
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Result.Message);
}
```

### Removing Items
Removal can target an explicit slot or just an item instance.
```cpp
// Remove 1 application of the item living in TypeID 0 (Backpack), Slot Index 4
InventoryComp->RemoveItemAt(0, 4, 1);
```

### Transferring Items (Crucial for UI Drag & Drop)
The system gracefully handles failures (e.g., trying to put a Sword into a Potion Slot). If it fails, the item natively snaps back to its origin.
```cpp
// Move from Backpack (0), Slot 2 --> to Hotbar (1), Slot 0.
FInventoryOperationResult SwapResult = InventoryComp->TransferItem(0, 2, 1, 0);
```

### Search & Utility
```cpp
// Sorting: Auto-consolidates stacks and moves items to the front of the array.
InventoryComp->OrganizeInventory();

// Validation: Check how many spaces are left.
int32 FreeSlots = InventoryComp->GetEmptySlotCount(0); // Checks only Backpack
```

---

## 6. Multiplayer & Networking Guide

The plugin is strictly designed to be server-authoritative and replication-friendly.

*   `UInventoryComponent` Replicates its variables and subobjects automatically.
*   `UItemBase` and internal modules use `ReplicateSubobjects()`.

**Important Rules for Multiplayer:**
1. **Never Call `AddItem` or `TransferItem` from the Client.**
2. If your UI (Client) wants to move an item, you fire a **Run on Server** Custom Event in your Player Controller or Character.
3. The Server calls `TransferItem(...)`.
4. The Replication engine automatically updates the Client's `FInventorySlotsGroup`.
5. The Client's UI binds to the Component's Multicast Delegates (`OnItemAdded`, `OnItemRemoved`, `OnItemStackChanged`) to update visually.

---

## 7. Advanced Customization: Writing Your Own Modules

The true power of this plugin unlocks when you write your own C++ logic.

### Creating a Weapon Equip Module
Create a new C++ class extending `UItemModuleBase`.
**WeaponEquipModule.h**
```cpp
UCLASS(Blueprintable)
class UWeaponEquipModule : public UItemModuleBase
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    float BaseDamage = 25.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    UAnimMontage* AttackAnimation;

    // Hook into inventory events!
    virtual void OnItemAddedToInventory_Implementation(AActor* Owner) override;
};
```
**WeaponEquipModule.cpp**
```cpp
void UWeaponEquipModule::OnItemAddedToInventory_Implementation(AActor* Owner)
{
    Super::OnItemAddedToInventory_Implementation(Owner);
    
    // Logic: If this item was dropped into a specific "Equipment" TypeID slot, 
    // spawn the physical Mesh on the Player's hand socket!
}
```

By keeping `BaseDamage` and `AttackAnimation` inside this module, items like "Bread" don't waste memory storing damage values. Only items with the `WeaponEquipModule` have these variables.

---

## 🛠️ Support & Contact
If you encounter bugs, logic flaws, or have feature requests, please reach out via the marketplace support page or the designated GitHub Issues tab. Thank you for integrating the Modular Inventory System into your world!
