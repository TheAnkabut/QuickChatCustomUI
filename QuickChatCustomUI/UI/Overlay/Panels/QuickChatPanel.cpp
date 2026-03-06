#include "pch.h"
#include "Panels.h"
#include "../Settings.h"
#include "../Layout.h"
#include "../../../RL/QuickChatConfig.h"
#include "../../../Overlay/Text.h"
#include "../../../Overlay/Fonts/FontSystem.h"
#include "../../../Utils/Helpers.h"
#include "../../../IMGUI/imgui_internal.h"
#include "../../../Utils/Localization.h"
#include <algorithm>

namespace QuickChatPanel
{
    static float splitterWidth = 180.0f;
    static constexpr float PANEL_HEIGHT = 220.0f;
    static int currentGroupIndex = 0;
    static bool syncGroup = false;

    void Render()
    {
        const auto& bindings = QuickChatConfig::GetBindings();
        
        float availableWidth = ImGui::GetContentRegionAvail().x;
        ImVec2 startPos = ImGui::GetCursorScreenPos();
        float rightWidth = Layout::HandleSplitter("##QCSplitter", splitterWidth, startPos, PANEL_HEIGHT, availableWidth);
        
        // Left panel: group selector and slot list
        ImGui::SetCursorScreenPos(ImVec2(startPos.x + 7, startPos.y));
        ImGui::BeginChild("##QCLeftPanel", ImVec2(splitterWidth - 7, PANEL_HEIGHT), false);
        
        // Group selector - build localized group names
        std::string group1 = std::string(L("group")) + " 1";
        std::string group2 = std::string(L("group")) + " 2";
        std::string group3 = std::string(L("group")) + " 3";
        std::string group4 = std::string(L("group")) + " 4";
        std::string group5 = std::string(L("group")) + " 5";
        const char* groupNames[] = {group1.c_str(), group2.c_str(), group3.c_str(), group4.c_str(), group5.c_str()};
        
        ImGui::SetNextItemWidth(100);
        if (ImGui::Combo("##GroupSelect", &currentGroupIndex, groupNames, 5))
        {
            Text::SetPreviewGroup(currentGroupIndex);
        }
        
        // Slot list for current group
        ImGui::BeginChild("##QCSlotList", ImVec2(0, 0), true);
        for (int i = 0; i < 4; i++)
        {
            int slotIndex = currentGroupIndex * 4 + i;
            std::string message = bindings[slotIndex].message;
            if (message.empty())
            {
                message = L("empty");
            }
            
            std::string label = message + "##qc" + std::to_string(slotIndex);
            if (ImGui::Selectable(label.c_str(), Settings::selectedQCSlotIndex == slotIndex))
            {
                Settings::selectedQCSlotIndex = slotIndex;
            }
        }
        ImGui::EndChild();
        ImGui::EndChild();
        
        static std::string lastMessage[20];
        for (int s = 0; s < 20 && s < (int)bindings.size(); s++)
        {
            const std::string& msg = bindings[s].message;
            if (lastMessage[s] != msg)
            {
                auto& sl = Settings::quickChatSlots[s];
                sl.customText[0] = '\0';
                if (!msg.empty())
                {
                    strncpy(sl.customText, msg.c_str(), sizeof(sl.customText) - 1);
                    sl.customText[sizeof(sl.customText) - 1] = '\0';
                }
                lastMessage[s] = msg;
            }
        }

        // Right panel: slot properties
        ImGui::SetCursorScreenPos(ImVec2(startPos.x + splitterWidth + 8 + 7, startPos.y));
        ImGui::BeginChild("##QCRightPanel", ImVec2(rightWidth, PANEL_HEIGHT), false);
        
        if (Settings::selectedQCSlotIndex >= 0 && Settings::selectedQCSlotIndex < 20)
        {
            Settings::QuickChatSlot& slot = Settings::quickChatSlots[Settings::selectedQCSlotIndex];
            int groupBaseIndex = currentGroupIndex * 4;
            
            // Get original QC message
            const std::string& message = bindings[Settings::selectedQCSlotIndex].message;
            
            // If custom text is empty, copy the original message to show in input
            if (slot.customText[0] == '\0' && !message.empty())
            {
                strncpy(slot.customText, message.c_str(), sizeof(slot.customText) - 1);
                slot.customText[sizeof(slot.customText) - 1] = '\0';
            }
            
            // Custom text input - shows actual text
            ImGui::SetNextItemWidth(200);
            if (ImGui::InputText("##CustomText", slot.customText, sizeof(slot.customText)))
            {
                // Text changed while editing
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                Helpers::SaveCurrentPreset();
            }
            
            // Font selector
            if (FontSystem::RenderSelector(slot.fontName, "qc"))
            {
                if (syncGroup)
                {
                    for (int i = 0; i < 20; i++)
                    {
                        Settings::quickChatSlots[i].fontName = slot.fontName;
                    }
                }
                Helpers::SaveCurrentPreset();
            }
            
            // Sync checkbox
            ImGui::Checkbox(L("sync"), &syncGroup);
            
            // Position
            float position[2] = {slot.offsetX, slot.offsetY};
            if (ImGui::DragFloat2(L("position"), position, 0.5f, -1000.0f, 1000.0f, "%.1f%%"))
            {
                float deltaX = position[0] - slot.offsetX;
                float deltaY = position[1] - slot.offsetY;
                
                if (syncGroup)
                {
                    for (int i = 0; i < 20; i++)
                    {
                        Settings::quickChatSlots[i].offsetX += deltaX;
                        Settings::quickChatSlots[i].offsetY += deltaY;
                    }
                }
                else
                {
                    slot.offsetX = position[0];
                    slot.offsetY = position[1];
                }
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                Helpers::SaveCurrentPreset();
            }
            
            // Font size
            if (ImGui::SliderFloat(L("size"), &slot.fontSize, 12.0f, 200.0f, "%.1f px"))
            {
                if (syncGroup)
                {
                    for (int i = 0; i < 20; i++)
                    {
                        Settings::quickChatSlots[i].fontSize = slot.fontSize;
                    }
                }
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                Helpers::SaveCurrentPreset();
            }
            
            // Opacity
            if (ImGui::SliderFloat(L("opacity"), &slot.opacity, 0.0f, 100.0f, "%.0f%%"))
            {
                if (syncGroup)
                {
                    for (int i = 0; i < 20; i++)
                    {
                        Settings::quickChatSlots[i].opacity = slot.opacity;
                    }
                }
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                Helpers::SaveCurrentPreset();
            }
            
            // Spacing 
            float baseOffsetY = Settings::quickChatSlots[groupBaseIndex].offsetY;
            float spacing = Settings::quickChatSlots[groupBaseIndex + 1].offsetY - baseOffsetY;
            if (spacing < 1.0f)
            {
                spacing = 15.0f;
            }
            
            if (ImGui::SliderFloat(L("spacing"), &spacing, 2.0f, 30.0f, "%.1f%%"))
            {
                if (syncGroup)
                {
                    // Apply to ALL 20 slots (5 groups x 4 slots)
                    for (int g = 0; g < 5; g++)
                    {
                        float groupBaseY = Settings::quickChatSlots[g * 4].offsetY;
                        for (int i = 0; i < 4; i++)
                        {
                            Settings::quickChatSlots[g * 4 + i].offsetY = groupBaseY + i * spacing;
                        }
                    }
                }
                else
                {
                    // Apply only to current group
                    for (int i = 0; i < 4; i++)
                    {
                        Settings::quickChatSlots[groupBaseIndex + i].offsetY = baseOffsetY + i * spacing;
                    }
                }
                Helpers::SaveCurrentPreset();
            }
            
            // Normal color 
            ImGui::Text(L("color"));
            ImGui::SameLine();
            if (ImGui::ColorEdit4("##QCColor", slot.color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar))
            {
                if (syncGroup)
                {
                    for (int i = 0; i < 20; i++)
                    {
                        memcpy(Settings::quickChatSlots[i].color, slot.color, sizeof(slot.color));
                    }
                }
                Helpers::SaveCurrentPreset();
            }
            
            // Selected color (when quickchat is sent)
            ImGui::Text(L("selected"));
            ImGui::SameLine();
            if (ImGui::ColorEdit4("##QCSelectedColor", slot.selectedColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar))
            {
                if (syncGroup)
                {
                    for (int i = 0; i < 20; i++)
                    {
                        memcpy(Settings::quickChatSlots[i].selectedColor, slot.selectedColor, sizeof(slot.selectedColor));
                    }
                }
                Helpers::SaveCurrentPreset();
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip(L("selected_tooltip"));
            }
            
            // Shadow
            if (ImGui::Checkbox(L("shadow"), &slot.shadow))
            {
                if (syncGroup)
                {
                    for (int i = 0; i < 20; i++)
                    {
                        Settings::quickChatSlots[i].shadow = slot.shadow;
                    }
                }
                Helpers::SaveCurrentPreset();
            }
            ImGui::SameLine();
            if (ImGui::ColorEdit4("##ShadowColor", slot.shadowColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar))
            {
                if (syncGroup)
                {
                    for (int i = 0; i < 20; i++)
                    {
                        memcpy(Settings::quickChatSlots[i].shadowColor, slot.shadowColor, sizeof(slot.shadowColor));
                    }
                }
                Helpers::SaveCurrentPreset();
            }
        }
        
        ImGui::EndChild();
        ImGui::SetCursorScreenPos(ImVec2(startPos.x, startPos.y + PANEL_HEIGHT + 5));
    }
}
