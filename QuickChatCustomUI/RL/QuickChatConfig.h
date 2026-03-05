#pragma once
#include <string>
#include <array>
#include <memory>
#include <functional>

namespace QuickChatConfig
{
    struct QuickChatBinding
    {
        std::string messageId;
        std::string message;
        int group = 0;
    };

    void Initialize(std::shared_ptr<GameWrapper> wrapper);
    bool IsLoaded();
    void SetLoaded(bool loaded);

    const std::array<QuickChatBinding, 24>& GetBindings();
    std::array<QuickChatBinding, 24>& GetMutableBindings();

    // Called when bindings are refreshed from game memory
    void SetOnBindingsUpdated(std::function<void()> callback);
}
