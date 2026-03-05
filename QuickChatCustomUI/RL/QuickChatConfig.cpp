#include "pch.h"
#include "QuickChatConfig.h"
#include "bakkesmod/wrappers/Engine/ActorWrapper.h"

namespace QuickChatConfig
{
    // ==================== RL Memory Structures ====================

    template<typename T>
    struct TArray
    {
        T* Data;
        int32_t Count;
        int32_t Max;
    };

    struct FString
    {
        wchar_t* Data;
        int32_t Count;
        int32_t Max;

        std::string ToUTF8() const
        {
            if (!Data || Count <= 0)
            {
                return "";
            }

            int length = WideCharToMultiByte(CP_UTF8, 0, Data, Count - 1, nullptr, 0, nullptr, nullptr);
            std::string result(length, '\0');
            WideCharToMultiByte(CP_UTF8, 0, Data, Count - 1, result.data(), length, nullptr, nullptr);
            return result;
        }
    };

    struct FLocalizedQuickChatBinding
    {
        FString MessageId;
        FString Message;
        int32_t Group;
        uint8_t Padding[4];
        FString Action;
    };

    // ==================== State ====================

    static std::shared_ptr<GameWrapper> gameWrapper;
    static std::array<QuickChatBinding, 24> bindings;
    static bool isLoaded = false;
    static uintptr_t cachedAddress = 0;
    static std::function<void()> onBindingsUpdated;

    static void ReadBindingsFromMemory(uintptr_t address);

    // ==================== Lifecycle ====================

    void Initialize(std::shared_ptr<GameWrapper> wrapper)
    {
        QuickChatConfig::gameWrapper = wrapper;

        wrapper->HookEventWithCallerPost<ActorWrapper>(
            "Function TAGame.GFxData_QuickChatBindings_TA.GenerateBindings",
            [](ActorWrapper caller, void*, std::string) {
                ReadBindingsFromMemory(caller.memory_address);
            }
        );

        gameWrapper->HookEventWithCallerPost<ActorWrapper>(
            "Function TAGame.GFxData_QuickChatBindings_TA.ChangeBinding",
            [](ActorWrapper caller, void*, std::string) {
                uintptr_t address = cachedAddress ? cachedAddress : caller.memory_address;
                ReadBindingsFromMemory(address);
            }
        );
    }

    bool IsLoaded()
    {
        return isLoaded;
    }

    void SetLoaded(bool loaded)
    {
        isLoaded = loaded;
    }

    const std::array<QuickChatBinding, 24>& GetBindings()
    {
        return bindings;
    }

    std::array<QuickChatBinding, 24>& GetMutableBindings()
    {
        return bindings;
    }

    void SetOnBindingsUpdated(std::function<void()> callback)
    {
        onBindingsUpdated = callback;
    }

    // ==================== Internal ====================

    static void ReadBindingsFromMemory(uintptr_t address)
    {
        if (!address)
        {
            return;
        }

        // Bindings array at offset 0xA8 from the caller
        auto* bindingsArray = reinterpret_cast<TArray<FLocalizedQuickChatBinding>*>(address + 0xA8);

        if (!bindingsArray->Data || bindingsArray->Count < 24)
        {
            return;
        }

        for (int i = 0; i < 24; i++)
        {
            bindings[i].messageId = bindingsArray->Data[i].MessageId.ToUTF8();
            bindings[i].message = bindingsArray->Data[i].Message.ToUTF8();
            bindings[i].group = bindingsArray->Data[i].Group;
        }

        cachedAddress = address;
        isLoaded = true;

        if (onBindingsUpdated)
        {
            onBindingsUpdated();
        }
    }
}
