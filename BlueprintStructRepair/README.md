# Blueprint Struct Repair
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
