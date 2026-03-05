#include "pch.h"
#include "Header.h"
#include "Layout.h"
#include "../../Persistence/Presets.h"
#include "../../RL/BlockUI.h"
#include "../../Overlay/Base.h"
#include "../../Utils/Localization.h"
#include "../../Utils/Helpers.h"
#include "Settings.h"

namespace Header
{
    static char newPresetName[128] = "";
    static bool isRenaming = false;
    static bool isCreating = false;
    static char renameBuffer[128] = "";

    void Render(std::shared_ptr<GameWrapper> gameWrapper, std::vector<std::string>& presetList, int& selectedIndex, bool& needRefresh)
    {
        // Sync OverlayBase with Settings values
        OverlayBase::GlobalOffsetX() = Settings::globalOffsetX;
        OverlayBase::GlobalOffsetY() = Settings::globalOffsetY;
        OverlayBase::GlobalScale() = Settings::globalScale;

        ImGui::Text(L("presets"));

        ImGui::SetNextItemWidth(200);

        if (selectedIndex < 0 || selectedIndex >= (int)presetList.size())
        {
            selectedIndex = presetList.empty() ? -1 : 0;
        }

        const char* previewLabel = L("none");
        if (selectedIndex >= 0 && selectedIndex < (int)presetList.size())
        {
            previewLabel = presetList[selectedIndex].c_str();
        }

        if (ImGui::BeginCombo("##PresetCombo", previewLabel))
        {
            for (int i = 0; i < (int)presetList.size(); i++)
            {
                bool isSelected = (selectedIndex == i);
                if (ImGui::Selectable(presetList[i].c_str(), isSelected))
                {
                    selectedIndex = i;
                    Presets::LoadPreset(presetList[i]);
                    Presets::SetConfigActivePreset(presetList[i]);
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        static bool needFocusCreate = false;
        static bool needFocusRename = false;

        ImGui::SameLine();
        if (ImGui::Button(L("new")) && !isCreating)
        {
            newPresetName[0] = '\0';
            isCreating = true;
            needFocusCreate = true;
        }

        ImGui::SameLine();
        if (ImGui::Button(L("rename")) && !isRenaming && selectedIndex >= 0 && selectedIndex < (int)presetList.size())
        {
            strncpy(renameBuffer, presetList[selectedIndex].c_str(), 127);
            isRenaming = true;
            needFocusRename = true;
        }

        ImGui::SameLine();
        if (ImGui::Button(L("delete")) && selectedIndex >= 0 && selectedIndex < (int)presetList.size())
        {
            Presets::DeletePreset(presetList[selectedIndex]);
            needRefresh = true;
            presetList = Presets::GetPresetList();
            if (presetList.empty())
            {
                selectedIndex = -1;
            }
            else
            {
                selectedIndex = 0;
                Presets::LoadPreset(presetList[0]);
                Presets::SetConfigActivePreset(presetList[0]);
            }
        }

        if (isCreating)
        {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120);
            if (needFocusCreate)
            {
                ImGui::SetKeyboardFocusHere();
                needFocusCreate = false;
            }
            bool confirmed = ImGui::InputText("##NewPresetName", newPresetName, 128, ImGuiInputTextFlags_EnterReturnsTrue);
            bool clickedOutside = ImGui::IsMouseClicked(0) && !ImGui::IsItemHovered();

            if ((confirmed || clickedOutside) && newPresetName[0] != '\0')
            {
                if (Presets::CreatePreset(newPresetName))
                {
                    needRefresh = true;
                    newPresetName[0] = '\0';
                }
                isCreating = false;
            }
            else if (clickedOutside)
            {
                isCreating = false;
            }
        }
        else if (isRenaming)
        {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120);
            if (needFocusRename)
            {
                ImGui::SetKeyboardFocusHere();
                needFocusRename = false;
            }
            bool confirmed = ImGui::InputText("##RenameInput", renameBuffer, 128, ImGuiInputTextFlags_EnterReturnsTrue);
            bool clickedOutside = ImGui::IsMouseClicked(0) && !ImGui::IsItemHovered();

            if ((confirmed || clickedOutside) && renameBuffer[0] != '\0' && selectedIndex >= 0 && selectedIndex < (int)presetList.size())
            {
                Presets::RenamePreset(presetList[selectedIndex], renameBuffer);
                needRefresh = true;
                isRenaming = false;
            }
            else if (clickedOutside)
            {
                isRenaming = false;
            }
        }

        Layout::DrawSeparator();

        // Per-group CustomUI toggles
        ImGui::Text(L("groups"));
        for (int i = 0; i < 5; i++)
        {
            if (i > 0) ImGui::SameLine();
            char label[32];
            snprintf(label, sizeof(label), "%s %d", L("group"), i + 1);
            if (ImGui::Checkbox(label, &Settings::enabledGroups[i]))
            {
                Helpers::SaveCurrentPreset();
            }
        }

        bool previousShow = Settings::showOverlay;
        ImGui::Checkbox(L("show_multimedia"), &Settings::showOverlay);

        if (Settings::showOverlay != previousShow)
        {
            gameWrapper->Execute([](GameWrapper*) {
                _globalCvarManager->executeCommand("togglemenu QuickChatCustomUI");
            });
        }

        float displayDuration = BlockUI::GetDisplayDuration();
        ImGui::SetNextItemWidth(360);
        if (ImGui::SliderFloat(L("duration"), &displayDuration, 0.5f, 10.0f, "%.1fs"))
        {
            BlockUI::SetDisplayDuration(displayDuration);
            Presets::SetConfigDuration(displayDuration);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(L("duration_tooltip"));
        }

        ImGui::Text(L("global"));
        ImGui::SetNextItemWidth(360);
        float globalOffset[2] = {Settings::globalOffsetX, Settings::globalOffsetY};
        if (ImGui::DragFloat2(L("position"), globalOffset, 0.5f, -100.0f, 100.0f, "%.1f%%"))
        {
            Settings::globalOffsetX = globalOffset[0];
            Settings::globalOffsetY = globalOffset[1];
            OverlayBase::GlobalOffsetX() = globalOffset[0];
            OverlayBase::GlobalOffsetY() = globalOffset[1];
        }

        ImGui::SetNextItemWidth(360);
        if (ImGui::SliderFloat(L("scale"), &Settings::globalScale, 0.1f, 5.0f, "x%.2f"))
        {
            OverlayBase::GlobalScale() = Settings::globalScale;
        }

        ImGui::Text(L("fade"));
        if (ImGui::RadioButton(L("linear"), Settings::fadeMode == Settings::FadeMode::LINEAR))
        {
            Settings::fadeMode = Settings::FadeMode::LINEAR;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton(L("smooth"), Settings::fadeMode == Settings::FadeMode::SMOOTH))
        {
            Settings::fadeMode = Settings::FadeMode::SMOOTH;
        }

        ImGui::SetNextItemWidth(360);
        ImGui::SliderFloat(L("fade_in"), &Settings::fadeInDuration, 0.0f, 0.5f, "%.2fs");
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(L("fade_in_tooltip"));
        }

        ImGui::SetNextItemWidth(360);
        ImGui::SliderFloat(L("fade_out"), &Settings::fadeOutSlow, 0.0f, 1.5f, "%.2fs");
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(L("fade_out_tooltip"));
        }

        ImGui::SetNextItemWidth(360);
        ImGui::SliderFloat("Timeout", &Settings::fadeOutFast, 0.0f, 0.5f, "%.2fs");
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(L("timeout_tooltip"));
        }
    }
}
