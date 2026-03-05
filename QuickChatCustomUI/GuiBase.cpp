#include "pch.h"
#include "GuiBase.h"
#include "RL/BlockUI.h"


extern std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

std::string SettingsWindowBase::GetPluginName()
{
    return "Quick Chat Custom UI";
}

void SettingsWindowBase::SetImGuiContext(uintptr_t ctx)
{
    ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

std::string PluginWindowBase::GetMenuName()
{
    return "QuickChatCustomUI";
}

std::string PluginWindowBase::GetMenuTitle()
{
    return menuTitle_;
}

void PluginWindowBase::SetImGuiContext(uintptr_t ctx)
{
    ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

bool PluginWindowBase::ShouldBlockInput()
{
    auto& io = ImGui::GetIO();
    return io.WantCaptureMouse || io.WantCaptureKeyboard;
}

bool PluginWindowBase::IsActiveOverlay()
{
    return true;
}

void PluginWindowBase::OnOpen()
{
    isWindowOpen_ = true;
}

void PluginWindowBase::OnClose()
{
    isWindowOpen_ = false;
    BlockUI::OnMenuClosed();
}

void PluginWindowBase::Render()
{
    if (!ImGui::Begin(menuTitle_.c_str(), &isWindowOpen_, ImGuiWindowFlags_None))
    {
        ImGui::End();
        return;
    }
    
    RenderWindow();
    ImGui::End();
    
    if (!isWindowOpen_)
    {
        _globalCvarManager->executeCommand("togglemenu " + GetMenuName());
    }
}
