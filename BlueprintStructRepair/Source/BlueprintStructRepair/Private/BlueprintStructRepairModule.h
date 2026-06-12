#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSpawnTabArgs;
class SDockTab;

class FBlueprintStructRepairModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedRef<SDockTab> SpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs);
	void RegisterMenus();
	void OpenPluginWindow();
};
