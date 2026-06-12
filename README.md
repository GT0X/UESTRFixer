# Blueprint Struct Repair

UE UI plugin. Look for it in the Unreal Editor **Tools** tab.

Blueprint Struct Repair helps find and repair Blueprint assets affected by broken or outdated <b>user-defined</b> struct references.

It helps with common Unreal Blueprint issues such as:

- structs causing invalid struct pins
- stale DataTable row structs
- Blueprint nodes that need to be refreshed after user-defined struct changes
- cook/package logs that are annoying to untangle manually

## What It Does

- Finds Blueprint assets by name or `/Game/...` path.
- Runs **Refresh All Nodes** on the selected Blueprint without manual log untangling or asset opening.
- Compiles the Blueprint after refreshing nodes.
- Shows whether the asset has unsaved changes.
- Lists user-defined structs used by the selected Blueprint.
- Shows where each struct asset is located.
- Parses build, cook, and package logs.
- Extracts assets with struct-related errors from logs.

## Installation

1. Download or clone this repository.
2. Copy the `BlueprintStructRepair` folder into your Unreal project:

```text
YourProject/
  Plugins/
    BlueprintStructRepair/
```

3. If your project does not have a `Plugins` folder yet, create it.
4. Open the `.uproject` file.
5. When Unreal asks to rebuild missing modules, confirm the rebuild.
6. Open the plugin window from:

```text
Tools -> Blueprint Struct Repair
```

If Unreal does not offer to rebuild automatically, regenerate project files and build the project from your IDE, or build the plugin with Unreal's `RunUAT BuildPlugin` command.

## Safety

1. Do backups.
2. Back up your `Content` folder, or at least the affected structure `.uasset` files before attempting to fix anything.
3. Take a deep breath and call pasta god through condom for safe connection.
4. POUSH THOSE PLUGIN BUTTONS.

This plugin might help and fix everything. If not, you already were doomed for eternity before this happened.

The plugin does not automatically save repaired assets. Review the result, make sure Blueprints compile cleanly, and save manually only when you are satisfied with the result.

If you'd like to support a dissolute lifestyle, you can support me here: https://dalink.to/gt0x or buy some fancy things for yourself.

Love You All.
