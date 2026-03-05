#include "pch.h"
#include "BlockUI.h"
#include "bakkesmod/wrappers/wrapperstructs.h"
#include "../UI/Overlay/Settings.h"
#include <Windows.h>
#include <chrono>
#include <vector>
#include <functional>
#include <string>
#include <memory>
#include <cstdint>
#include <unordered_map>
#include <mutex>

namespace BlockUI
{
    // ==================== State ====================

    // ~60fps animation tick
    static constexpr float FRAME_TIME = 0.016f;

    // Key simulation timing - events fire in ~8ms, 20ms gives safe margin
    static constexpr float SIMULATION_RESET_DELAY = 0.02f;
    static constexpr float INJECTED_KEY_EXPIRY = 0.5f;

    // RL key indices: '1' = 15270, '2' = 15271, etc.
    static constexpr int64_t KEY_INDEX_BASE = 15270;

    static std::shared_ptr<GameWrapper> gameWrapper;

    // Overlay animation state machine
    enum class OverlayState { CLOSED, FADING_IN, VISIBLE, FADING_OUT };
    static OverlayState overlayState = OverlayState::CLOSED;
    static float overlayOpacity = 0.0f;
    static int sessionId = 0;
    static float currentFadeOutDuration = 0.8f;

    // Time-based animation (linear interpolation)
    static float animStartOpacity = 0.0f;
    static float animTargetOpacity = 0.0f;
    static float animElapsed = 0.0f;
    static float animDuration = 0.0f;

    // Overlay configuration
    static float displayDuration = 1.75f;
    static bool noAutoFade = false;
    static bool matchEndedOverride = false;

    // QuickChat selection state (two-click system: group then slot)
    static bool waitingForSlot = false;
    static int pendingGroup = -1;
    static bool nativeQCActive = false;  // RL's native QC wheel is handling a group
    static std::chrono::steady_clock::time_point nativeQCActiveTime;

    // Key simulation state to allow exactly 2 events through (group + slot)
    static bool simulationActive = false;
    static int allowedEvents = 0;
    static bool simulationPending = false;

    // Injected key tracking - prevents HandleKeyPress from double-processing simulated keys
    static std::unordered_map<int64_t, std::chrono::steady_clock::time_point> injectedKeyExpiry;
    static std::mutex injectedKeyMutex;

    // RL HandleKeyPress params structure 
    struct HandleKeyPressParams
    {
        int32_t ControllerId;
        int32_t KeyIndex;
        int32_t KeyNumber;
        uint8_t EventType;
        uint8_t padding[3];
        float AmountDepressed;
        uint32_t bGamepad : 1;
        bool ReturnValue : 1;
    };

    // ==================== Forward Declarations ====================

    static void OnHandleKeyPress(void* params);
    static void AnimationTick();
    static void ShowOverlay();
    static void HideOverlay(bool fastFade = false);
    static void StartAutoCloseTimer();
    static void OnChatPresetEvent(void* params);

    // ==================== Lifecycle ====================

    void Initialize(std::shared_ptr<GameWrapper> wrapper)
    {
        BlockUI::gameWrapper = wrapper;

        wrapper->HookEventWithCaller<ActorWrapper>(
            "Function TAGame.GameViewportClient_TA.HandleKeyPress",
            [](ActorWrapper, void* params, std::string) {
                OnHandleKeyPress(params);
            }
        );

        gameWrapper->HookEventWithCaller<ActorWrapper>(
            "Function TAGame.GFxHUD_TA.ChatPreset",
            [](ActorWrapper, void* params, std::string) {
                OnChatPresetEvent(params);
            }
        );

        // Match state hooks - track when match ends for group 4 override
        gameWrapper->HookEvent(
            "Function TAGame.GameEvent_Soccar_TA.OnMatchEnded",
            [](std::string) {
                matchEndedOverride = true;
            }
        );

        gameWrapper->HookEvent(
            "Function TAGame.GFxData_Chat_TA.InitializeQuickChat",
            [](std::string) {
                matchEndedOverride = false;
            }
        );
    }

    void Shutdown()
    {
        gameWrapper->UnhookEvent("Function TAGame.GameViewportClient_TA.HandleKeyPress");
        gameWrapper->UnhookEvent("Function TAGame.GFxHUD_TA.ChatPreset");
        gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.OnMatchEnded");
        gameWrapper->UnhookEvent("Function TAGame.GFxData_Chat_TA.InitializeQuickChat");
    }

    // ==================== Public API ====================

    float GetAnimatedOpacity()
    {
        return Settings::opacity * overlayOpacity;
    }

    float GetFadeOpacity()
    {
        if (overlayState == OverlayState::CLOSED)
        {
            return 1.0f;
        }
        return overlayOpacity;
    }

    bool IsGifActive()
    {
        return overlayState != OverlayState::CLOSED;
    }

    float GetDisplayDuration()
    {
        return displayDuration;
    }

    void SetDisplayDuration(float seconds)
    {
        displayDuration = seconds;
    }

    bool GetNoAutoFade()
    {
        return noAutoFade;
    }

    void SetNoAutoFade(bool value)
    {
        noAutoFade = value;
    }

    void OnMenuClosed()
    {
        if (overlayState != OverlayState::CLOSED)
        {
            overlayState = OverlayState::CLOSED;
            overlayOpacity = 0.0f;
            waitingForSlot = false;
            pendingGroup = -1;
            sessionId++;
        }
    }

    // ==================== Key Injection ====================

    static void CleanExpiredInjectedKeys()
    {
        auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(injectedKeyMutex);

        for (auto it = injectedKeyExpiry.begin(); it != injectedKeyExpiry.end(); )
        {
            if (it->second <= now)
            {
                it = injectedKeyExpiry.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    static void MarkKeyAsInjected(int64_t keyIndex)
    {
        auto expiryTime = std::chrono::steady_clock::now() +
            std::chrono::milliseconds(static_cast<int>(INJECTED_KEY_EXPIRY * 1000.0f));

        std::lock_guard<std::mutex> lock(injectedKeyMutex);
        injectedKeyExpiry[keyIndex] = expiryTime;
    }

    static bool IsInjectedKey(int64_t keyIndex)
    {
        CleanExpiredInjectedKeys();
        std::lock_guard<std::mutex> lock(injectedKeyMutex);
        return injectedKeyExpiry.find(keyIndex) != injectedKeyExpiry.end();
    }

    static int64_t VirtualKeyToKeyIndex(int virtualKey)
    {
        return static_cast<int64_t>(KEY_INDEX_BASE) + (virtualKey - '1');
    }

    // Consume injected key events to prevent double-processing by RL
    static void OnHandleKeyPress(void* params)
    {
        if (!params)
        {
            return;
        }

        auto* keyData = reinterpret_cast<HandleKeyPressParams*>(params);
        int64_t keyIndex = static_cast<int64_t>(keyData->KeyIndex);

        if (IsInjectedKey(keyIndex))
        {
            keyData->ReturnValue = true;
            return;
        }
    }

    // ==================== Animation ====================

    static void AnimationTick()
    {
        animElapsed += FRAME_TIME;
        float progress = animElapsed / animDuration;
        if (progress >= 1.0f)
        {
            progress = 1.0f;
        }

        overlayOpacity = animStartOpacity + (animTargetOpacity - animStartOpacity) * progress;

        if (overlayState == OverlayState::FADING_IN)
        {
            if (progress >= 1.0f)
            {
                overlayOpacity = 1.0f;
                overlayState = OverlayState::VISIBLE;
                return;
            }
            gameWrapper->SetTimeout([](GameWrapper*) {
                AnimationTick();
            }, FRAME_TIME);
        }
        else if (overlayState == OverlayState::FADING_OUT)
        {
            if (progress >= 1.0f)
            {
                overlayOpacity = 0.0f;
                overlayState = OverlayState::CLOSED;
                Settings::showOverlay = false;
                Settings::SetActiveQCGroup(-1);
                Settings::SetBoldSlot(-1);
                _globalCvarManager->executeCommand("togglemenu QuickChatCustomUI");
                return;
            }
            gameWrapper->SetTimeout([](GameWrapper*) {
                AnimationTick();
            }, FRAME_TIME);
        }
    }

    static void ShowOverlay()
    {
        Settings::SetBoldSlot(-1);

        // If already fading out, reverse from current opacity
        if (overlayState == OverlayState::FADING_OUT)
        {
            overlayState = OverlayState::FADING_IN;
            animStartOpacity = overlayOpacity;
            animTargetOpacity = 1.0f;
            animElapsed = 0.0f;
            animDuration = Settings::fadeInDuration;
            gameWrapper->SetTimeout([](GameWrapper*) {
                AnimationTick();
            }, FRAME_TIME);
            return;
        }

        if (overlayState != OverlayState::CLOSED)
        {
            return;
        }

        overlayState = OverlayState::FADING_IN;
        overlayOpacity = 0.0f;
        animStartOpacity = 0.0f;
        animTargetOpacity = 1.0f;
        animElapsed = 0.0f;
        animDuration = Settings::fadeInDuration;
        Settings::showOverlay = true;

        gameWrapper->Execute([](GameWrapper*) {
            _globalCvarManager->executeCommand("togglemenu QuickChatCustomUI");
        });

        gameWrapper->SetTimeout([](GameWrapper*) {
            AnimationTick();
        }, FRAME_TIME);
    }

    static void HideOverlay(bool fastFade)
    {
        if (overlayState != OverlayState::VISIBLE && overlayState != OverlayState::FADING_IN)
        {
            // Already fading out or closed - only allow switching to slow fade
            if (overlayState == OverlayState::FADING_OUT && !fastFade)
            {
                currentFadeOutDuration = Settings::fadeOutSlow;
            }
            return;
        }

        // If selection is pending, ignore autoclose timer's fast fade request
        if (fastFade && simulationPending)
        {
            return;
        }

        // No fade mode - instant close
        if (noAutoFade)
        {
            overlayOpacity = 0.0f;
            overlayState = OverlayState::CLOSED;
            Settings::showOverlay = false;
            Settings::SetActiveQCGroup(-1);
            Settings::SetBoldSlot(-1);
            _globalCvarManager->executeCommand("togglemenu QuickChatCustomUI");
            return;
        }

        // Slow fade for selection, fast fade for timeout
        currentFadeOutDuration = fastFade ? Settings::fadeOutFast : Settings::fadeOutSlow;

        overlayState = OverlayState::FADING_OUT;
        animStartOpacity = overlayOpacity;
        animTargetOpacity = 0.0f;
        animElapsed = 0.0f;
        animDuration = currentFadeOutDuration;
        gameWrapper->SetTimeout([](GameWrapper*) {
            AnimationTick();
        }, FRAME_TIME);
    }

    // ==================== Key Simulation ====================

    static INPUT BuildKeyInput(UINT virtualKey, UINT scanCode, bool keyDown)
    {
        INPUT input = {};
        input.type = INPUT_KEYBOARD;

        if (scanCode != 0)
        {
            input.ki.wVk = 0;
            input.ki.wScan = static_cast<WORD>(scanCode);
            input.ki.dwFlags = KEYEVENTF_SCANCODE | (keyDown ? 0 : KEYEVENTF_KEYUP);
        }
        else
        {
            input.ki.wVk = static_cast<WORD>(virtualKey);
            input.ki.dwFlags = keyDown ? 0 : KEYEVENTF_KEYUP;
        }

        input.ki.dwExtraInfo = GetMessageExtraInfo();
        return input;
    }

    static void SendQuickChatKeys(int groupKey, int slotKey)
    {
        UINT scanCode1 = MapVirtualKey(static_cast<UINT>(groupKey), MAPVK_VK_TO_VSC);
        UINT scanCode2 = MapVirtualKey(static_cast<UINT>(slotKey), MAPVK_VK_TO_VSC);

        MarkKeyAsInjected(VirtualKeyToKeyIndex(groupKey));
        MarkKeyAsInjected(VirtualKeyToKeyIndex(slotKey));

        // Release any held keys first to avoid stuck key issues
        INPUT cleanup[2] = {
            BuildKeyInput(groupKey, scanCode1, false),
            BuildKeyInput(slotKey, scanCode2, false)
        };
        SendInput(2, cleanup, sizeof(INPUT));

        // Atomic key sequence: group down, group up, slot down, slot up
        INPUT sequence[4] = {
            BuildKeyInput(groupKey, scanCode1, true),
            BuildKeyInput(groupKey, scanCode1, false),
            BuildKeyInput(slotKey, scanCode2, true),
            BuildKeyInput(slotKey, scanCode2, false)
        };
        SendInput(4, sequence, sizeof(INPUT));
    }

    // ==================== Event Handlers ====================

    static void StartAutoCloseTimer()
    {
        sessionId++;
        int currentSession = sessionId;

        gameWrapper->SetTimeout([currentSession](GameWrapper*) {
            if (currentSession != sessionId)
            {
                return;
            }

            if (waitingForSlot)
            {
                waitingForSlot = false;
                pendingGroup = -1;
                HideOverlay(true);
            }
        }, displayDuration + Settings::fadeInDuration);
    }

    static void OnChatPresetEvent(void* params)
    {
        if (!params)
        {
            return;
        }

        int32_t* groupPtr = reinterpret_cast<int32_t*>(params);
        int selectedValue = *groupPtr;

        // During simulation: allow exactly 2 events through (our injected group + slot keys)
        if (simulationActive)
        {
            if (allowedEvents > 0)
            {
                allowedEvents--;
                return;
            }

            *groupPtr = -1;
            return;
        }

        // Block during pending simulation (between user click and SendInput execution)
        if (simulationPending)
        {
            *groupPtr = -1;
            return;
        }

        // Block user input and process selection
        *groupPtr = -1;

        if (!waitingForSlot)
        {
            // Native RL QC is open - let it handle the slot event
            if (nativeQCActive)
            {
                // If >1.75s passed, user cancelled the native QC
                auto elapsed = std::chrono::duration<double>(
                    std::chrono::steady_clock::now() - nativeQCActiveTime
                ).count();
                if (elapsed > 1.75)
                {
                    nativeQCActive = false;  // Stale, treat as new group
                }
                else
                {
                    *groupPtr = selectedValue;
                    nativeQCActive = false;
                    return;
                }
            }

            // If CustomUI is disabled for this group, let RL handle it
            int checkGroup = selectedValue;
            if (matchEndedOverride && selectedValue == 0)
            {
                checkGroup = 4;
            }
            int enableIndex = (checkGroup >= 0 && checkGroup < 5) ? checkGroup : 0;
            if (!Settings::enabledGroups[enableIndex])
            {
                *groupPtr = selectedValue;  // Restore original value for RL
                nativeQCActive = true;
                nativeQCActiveTime = std::chrono::steady_clock::now();
                return;  // Pass through to native RL UI
            }

            // FIRST CLICK: Save group, show overlay, start autoclose timer
            pendingGroup = selectedValue;
            waitingForSlot = true;

            // When match ended, group 0 shows group 4 
            int displayGroup = selectedValue;
            if (matchEndedOverride && selectedValue == 0)
            {
                displayGroup = 4;
            }

            Settings::SetActiveQCGroup(displayGroup);
            ShowOverlay();
            StartAutoCloseTimer();
        }
        else
        {
            // SECOND CLICK: Execute quick chat, then hide overlay with slow fade
            int slotIndex = selectedValue;
            int groupIndex = pendingGroup;
            int groupKey = '1' + groupIndex;
            int slotKey = '1' + slotIndex;

            sessionId++;

            // Visual feedback on selected slot
            int activeGroup = Settings::activeQCGroup;
            if (activeGroup >= 0 && activeGroup < 5 && slotIndex >= 0 && slotIndex < 4)
            {
                Settings::SetBoldSlot(activeGroup * 4 + slotIndex);
            }

            simulationPending = true;

            gameWrapper->SetTimeout([groupKey, slotKey](GameWrapper*) {
                waitingForSlot = false;
                pendingGroup = -1;

                simulationActive = true;
                allowedEvents = 2;

                SendQuickChatKeys(groupKey, slotKey);

                // Reset simulation state after events have fired (~8ms, 20ms)
                gameWrapper->SetTimeout([](GameWrapper*) {
                    simulationActive = false;
                    allowedEvents = 0;
                    simulationPending = false;
                    HideOverlay(false);
                }, SIMULATION_RESET_DELAY);

            }, 0.0f);
        }
    }
}