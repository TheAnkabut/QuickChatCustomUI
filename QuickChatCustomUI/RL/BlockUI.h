#pragma once
#include <memory>

// GuiBase.cpp PluginWindowBase::OnClose() calls BlockUI::OnMenuClosed()

namespace BlockUI
{
    void Initialize(std::shared_ptr<GameWrapper> wrapper);
    void Shutdown();

    bool IsGifActive();

    float GetAnimatedOpacity();
    float GetFadeOpacity();

    float GetDisplayDuration();
    void SetDisplayDuration(float seconds);
    bool GetNoAutoFade();
    void SetNoAutoFade(bool value);

    void OnMenuClosed();
}
