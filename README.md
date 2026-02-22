# 🎒 Modular Inventory System for Unreal Engine

A highly extensible, multiplayer-ready, component-based inventory system built for Unreal Engine. It uses a **Composition-over-Inheritance** architecture, allowing you to build complex item behaviors using reusable modules instead of deep inheritance trees.

## ✨ Features

- **🧩 Component-Based Architecture (Modules)**: Attach functionality (e.g., *Consumable*, *Equippable*, *Durability*) to items dynamically using **Item Modules**. Avoid subclass bloat!
- **🌐 Multiplayer Ready**: Built with replication in mind from the ground up. Optimized structuring using caching and `TArray` for seamless network synchronization.
- **🛡️ Type-Safety & Error Handling**: All operations (`AddItem`, `TransferItem`, etc.) return a detailed `FInventoryOperationResult` struct for exact feedback.
- **📦 Stack Management**: Auto-stacking, merging, splitting, and limiting stack sizes native to the system.
- **📊 Data-Driven Design**: Base item definitions are driven by structures (`FItemDefinition`), making it incredibly easy to hook up to Data Tables.
- **🔄 Flexible Slots**: Support for distinct slot groups (e.g., Main Inventory, Hotbar, Equipment Slots) using custom `TypeIDs`.

---

## 🚀 Installation

1. Close your Unreal Engine project.
2. Clone or copy the `InventorySystem` folder into the `Plugins/` folder of your project. If the folder doesn't exist, create it.
3. Open your project. When prompted, click **Yes** to rebuild the plugin modules.
4. Go to **Edit > Plugins**, search for **InventorySystem**, and verify it is enabled.

---

## 📖 Quick Start Guide

### 1. Adding Inventory to a Character
1. Open your Character blueprint.
2. Add an **InventoryComponent** to the components list.
3. In the component properties, define your `InventorySlotsGroup`. You can set up how many slots the inventory has and assign `TypeIDs` (e.g., 0 for Main Bag, 1 for Hotbar).

### 2. Creating a Custom Item
1. Create a Blueprint inheriting from `UItemBase` (e.g., `BP_BaseItem`).
2. Inside its default properties, setup the **Item Definition** (Name, Icon, Description).
3. Under **Item Modules**, add any behaviors you need (e.g., `BP_ItemModule_Consumable`).

### 3. Adding Items to the Inventory
You can add items through C++ or Blueprint:
```cpp
// Create an instance of the item
UItemBase* NewItem = InventoryComp->CreateItemInstance(ItemClassTemplate);

// Add to inventory (Pass -1 to let the system auto-assign based on type)
FInventoryOperationResult Result = InventoryComp->AddItem(NewItem, -1);

if (Result.bSuccess)
{
    UE_LOG(LogTemp, Log, TEXT("Item added successfully!"));
}
else
{
    UE_LOG(LogTemp, Warning, TEXT("Failed: %s"), *Result.Message);
}
```

### 4. Transferring & Splitting Items
Transfer items between different slots or even entirely different inventories.
```cpp
// Transfers item from Slot 0 to Slot 5
InventoryComp->TransferItem(SourceTypeID, 0, DestTypeID, 5);

// Split a stack in half
UItemBase* SplitStack = MyItem->SplitStack(MyItem->GetCurrentStackSize() / 2);
```

---

## 🏛️ Core Architecture

### `UInventoryComponent`
The main actor component that lives on your Player, Chest, or Container. It manages `FInventorySlotsGroup` and fires delegates (`OnItemAdded`, `OnItemRemoved`, etc.) whenever the inventory changes.

### `UItemBase`
The core item object. It mostly acts as a container for its **Data** and its **Modules**. You rarely need to inherit from this.

### `UItemModuleBase`
The secret sauce. You inherit from this to create specific logic.
*Example: `UWearableModule`*, *`UConsumableModule`*, *`UQuestItemModule`*
By combining modules, you can create a sword that is both equippable and has durability, simply by adding both modules to the `ItemBase`.

### `UInventoryModuleBase`
Inventories can have modules too! For instance, an `AutoSortModule` or a `WeightLimitModule` could be attached to the `InventoryComponent`.

---

## 📡 Networking Considerations

This plugin supports replication out-of-the-box. Items and their inner modules utilize `ReplicateSubobjects`. Fast and reliable. Just ensure the Actor that owns the `InventoryComponent` is set to Replicate.

---

## ⚖️ License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
