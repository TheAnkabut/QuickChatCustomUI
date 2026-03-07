#include "pch.h"
#include "Settings.h"
#include "Welcome.h"
#include "Header.h"
#include "Layout.h"
#include "Panels/Panels.h"
#include "../../RL/QuickChatConfig.h"
#include "../../Overlay/Text.h"
#include "../../Persistence/Presets.h"
#include "../../Persistence/DirectoryWatcher.h"
#include "../../Utils/Localization.h"

namespace Settings
{
    static void RenderTextSection();
    
    static std::shared_ptr<GameWrapper> gameWrapper;
    
    bool showOverlay = false;
    float opacity = 100.0f;
    float globalOffsetX = 0.0f;
    float globalOffsetY = 0.0f;
    float globalScale = 1.0f;
    FadeMode fadeMode = FadeMode::LINEAR;
    float fadeInDuration = 0.25f;
    float fadeOutSlow = 0.8f;
    float fadeOutFast = 0.2f;
    std::vector<MediaObject> mediaObjects;
    int selectedMediaIndex = -1;
    std::vector<TextObject> textObjects;
    int selectedTextIndex = 0;
    std::array<QuickChatSlot, 20> quickChatSlots;
    int selectedQCSlotIndex = 0;
    int activeQCGroup = -1;
    int boldSlotIndex = -1;
    bool enabledGroups[5] = {true, true, true, true, true};
    
    void SetActiveQCGroup(int group)
    {
        if (group >= 0 && group <= 4)
        {
            activeQCGroup = group;
        }
        else
        {
            activeQCGroup = -1;
        }
    }
    
    void SetBoldSlot(int slot)
    {
        if (slot >= 0 && slot < 20)
        {
            boldSlotIndex = slot;
        }
        else
        {
            boldSlotIndex = -1;
        }
    }
    
    void Initialize(std::shared_ptr<GameWrapper> wrapper)
    {
        Settings::gameWrapper = wrapper;
    }
    
    void Render()
    {
        static int selectedPresetIndex = -1;
        static std::vector<std::string> presetList;
        static bool needRefresh = true;
        static bool watcherStarted = false;
        static bool* refreshPtr = &needRefresh;
        
        // Detect external changes to preset files 
        if (!watcherStarted)
        {
            DirectoryWatcher::Start(Presets::GetPresetsFolder(), []() {
                *refreshPtr = true;
            });
            watcherStarted = true;
        }
        
        DirectoryWatcher::IsRunning();
        
        if (needRefresh)
        {
            presetList = Presets::GetPresetList();
            needRefresh = false;
            
            std::string currentName = Presets::GetCurrentPresetName();
            selectedPresetIndex = -1;
            for (int i = 0; i < (int)presetList.size(); i++)
            {
                if (presetList[i] == currentName)
                {
                    selectedPresetIndex = i;
                    break;
                }
            }
            
            if (selectedPresetIndex < 0 && !presetList.empty())
            {
                selectedPresetIndex = 0;
                Presets::LoadPreset(presetList[0]);
            }
        }
        
        if (presetList.empty())
        {
            Welcome::Render();
            needRefresh = true;
            return;
        }
        
        if (!QuickChatConfig::IsLoaded())
        {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), L("quickchat_instruction"));
            ImGui::TextWrapped(L("open_settings"));
            return;
        }
        
        Header::Render(gameWrapper, presetList, selectedPresetIndex, needRefresh);
        MediaPanel::Render();
        RenderTextSection();
    }
    
    void RenderTextSection()
    {
        if (!ImGui::CollapsingHeader(L("quickchat_texts")))
        {
            return;
        }
        
        static int selectedTab = 0;
        
        if (ImGui::BeginTabBar("##TextTabs"))
        {
            if (ImGui::BeginTabItem(L("quickchats")))
            {
                if (selectedTab != 0)
                {
                    selectedTab = 0;
                    Text::SetPreviewGroup(0);
                }
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem(L("more_texts")))
            {
                selectedTab = 1;
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
        
        if (selectedTab == 0)
        {
            QuickChatPanel::Render();
        }
        else
        {
            TextPanel::Render();
        }
    }
}
