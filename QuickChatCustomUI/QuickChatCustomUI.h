#pragma once
#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "version.h"

constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

class QuickChatCustomUI : public BakkesMod::Plugin::BakkesModPlugin, 
                          public SettingsWindowBase, 
                          public PluginWindowBase
{
public:
    void onLoad() override;
    void onUnload() override;
    
    void RenderSettings() override;
    
    void Render() override;
    void RenderWindow() override {}

private:
    void SendCount();
};
