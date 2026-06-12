#include "BlueprintStructRepairModule.h"

#include "BlueprintStructRepairWidget.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/Docking/TabManager.h"
#include "Textures/SlateIcon.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FBlueprintStructRepairModule"

namespace
{
	const FName BlueprintStructRepairTabName("BlueprintStructRepair");
}

void FBlueprintStructRepairModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		BlueprintStructRepairTabName,
		FOnSpawnTab::CreateRaw(this, &FBlueprintStructRepairModule::SpawnPluginTab))
		.SetDisplayName(LOCTEXT("TabTitle", "Blueprint Struct Repair"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FBlueprintStructRepairModule::RegisterMenus));
}

void FBlueprintStructRepairModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(BlueprintStructRepairTabName);
}

TSharedRef<SDockTab> FBlueprintStructRepairModule::SpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SBlueprintStructRepairWidget)
		];
}

void FBlueprintStructRepairModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	FToolMenuSection& Section = Menu->FindOrAddSection("BlueprintStructRepair");
	Section.AddMenuEntry(
		"OpenBlueprintStructRepair",
		LOCTEXT("OpenWindowLabel", "Blueprint Struct Repair"),
		LOCTEXT("OpenWindowTooltip", "Open the Blueprint Struct Repair utility window."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FBlueprintStructRepairModule::OpenPluginWindow)));
}

void FBlueprintStructRepairModule::OpenPluginWindow()
{
	FGlobalTabmanager::Get()->TryInvokeTab(BlueprintStructRepairTabName);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBlueprintStructRepairModule, BlueprintStructRepair)
