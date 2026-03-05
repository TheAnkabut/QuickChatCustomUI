#include "pch.h"
#include "Panels.h"
#include "../Settings.h"
#include "../Layout.h"
#include "../../../Overlay/Fonts/FontSystem.h"
#include "../../../Utils/Helpers.h"
#include "../../../IMGUI/imgui_internal.h"
#include "../../../Utils/Localization.h"
#include <algorithm>

namespace TextPanel
{
    static float splitterWidth = 180.0f;
    static constexpr float PANEL_HEIGHT = 220.0f;
    static bool syncAll = false;

    void Render()
    {
        float availableWidth = ImGui::GetContentRegionAvail().x;
        ImVec2 startPos = ImGui::GetCursorScreenPos();
        float rightWidth = Layout::HandleSplitter("##TextSplitter", splitterWidth, startPos, PANEL_HEIGHT, availableWidth);
        
        // Left panel: text list
        ImGui::SetCursorScreenPos(ImVec2(startPos.x + 7, startPos.y));
        ImGui::BeginChild("##TextLeftPanel", ImVec2(splitterWidth - 7, PANEL_HEIGHT), false);
        
        // Add button
        if (ImGui::Button("+##AddText"))
        {
            Settings::textObjects.push_back(Settings::TextObject());
            Settings::selectedTextIndex = (int)Settings::textObjects.size() - 1;
            Helpers::SaveCurrentPreset();
        }
        
        ImGui::SameLine();
        
        // Remove button
        if (ImGui::Button("-##RemoveText") && 
            !Settings::textObjects.empty() &&
            Settings::selectedTextIndex >= 0 &&
            Settings::selectedTextIndex < (int)Settings::textObjects.size())
        {
            Settings::textObjects.erase(Settings::textObjects.begin() + Settings::selectedTextIndex);
            if (Settings::selectedTextIndex >= (int)Settings::textObjects.size())
            {
                Settings::selectedTextIndex = (int)Settings::textObjects.size() - 1;
            }
            Helpers::SaveCurrentPreset();
        }
        
        // Text list with inline editing
        ImGui::BeginChild("##TextList", ImVec2(0, 0), true);
        for (int i = 0; i < (int)Settings::textObjects.size(); i++)
        {
            ImGui::PushID(i);
            
            bool isSelected = (Settings::selectedTextIndex == i);
            if (ImGui::RadioButton("##Select", isSelected))
            {
                Settings::selectedTextIndex = i;
            }
            
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##Content", Settings::textObjects[i].content, sizeof(Settings::textObjects[i].content));
            
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                Helpers::SaveCurrentPreset();
            }
            if (ImGui::IsItemActive())
            {
                Settings::selectedTextIndex = i;
            }
            
            ImGui::PopID();
        }
        ImGui::EndChild();
        ImGui::EndChild();
        
        // Right panel: text properties
        ImGui::SetCursorScreenPos(ImVec2(startPos.x + splitterWidth + 8 + 7, startPos.y));
        ImGui::BeginChild("##TextRightPanel", ImVec2(rightWidth, PANEL_HEIGHT), false);
        
        if (Settings::selectedTextIndex >= 0 && Settings::selectedTextIndex < (int)Settings::textObjects.size())
        {
            Settings::TextObject& text = Settings::textObjects[Settings::selectedTextIndex];
            
            // Font selector
            if (FontSystem::RenderSelector(text.fontName))
            {
                if (syncAll)
                {
                    for (auto& t : Settings::textObjects)
                    {
                        t.fontName = text.fontName;
                    }
                }
                Helpers::SaveCurrentPreset();
            }
            
            // Sync checkbox
            ImGui::Checkbox(L("sync"), &syncAll);
            
            // Position
            float position[2] = {text.offsetX, text.offsetY};
            if (ImGui::DragFloat2(L("position"), position, 0.5f, -1000.0f, 1000.0f, "%.1f%%"))
            {
                float deltaX = position[0] - text.offsetX;
                float deltaY = position[1] - text.offsetY;
                
                if (syncAll && Settings::textObjects.size() > 1)
                {
                    for (auto& t : Settings::textObjects)
                    {
                        t.offsetX += deltaX;
                        t.offsetY += deltaY;
                    }
                }
                else
                {
                    text.offsetX = position[0];
                    text.offsetY = position[1];
                }
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                Helpers::SaveCurrentPreset();
            }
            
            // Font size
            if (ImGui::SliderFloat(L("size"), &text.fontSize, 12.0f, 1000.0f, "%.1f px"))
            {
                if (syncAll && Settings::textObjects.size() > 1)
                {
                    for (auto& t : Settings::textObjects)
                    {
                        t.fontSize = text.fontSize;
                    }
                }
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                Helpers::SaveCurrentPreset();
            }
            
            // Opacity
            if (ImGui::SliderFloat(L("opacity"), &text.opacity, 0.0f, 100.0f, "%.0f%%"))
            {
                if (syncAll && Settings::textObjects.size() > 1)
                {
                    for (auto& t : Settings::textObjects)
                    {
                        t.opacity = text.opacity;
                    }
                }
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                Helpers::SaveCurrentPreset();
            }
            
            // Group selector with checkboxes (multi-select) 
            ImGui::Text(L("qc_group"));
            ImGui::SameLine();
            for (int g = 0; g < 5; g++)
            {
                bool checked = (text.groupMask & (1 << g)) != 0;
                char label[8];
                sprintf(label, "%d##tg%d", g + 1, g);
                if (ImGui::Checkbox(label, &checked))
                {
                    if (checked)
                    {
                        text.groupMask |= (1 << g);
                    }
                    else
                    {
                        text.groupMask &= ~(1 << g);
                    }
                    
                    if (syncAll && Settings::textObjects.size() > 1)
                    {
                        for (auto& t : Settings::textObjects)
                        {
                            t.groupMask = text.groupMask;
                        }
                    }
                    Helpers::SaveCurrentPreset();
                }
                if (g < 3)
                {
                    ImGui::SameLine();
                }
            }
            if (text.groupMask == 0)
            {
                ImGui::SameLine();
                ImGui::TextDisabled("(%s)", L("all"));
            }
            
            // Color
            ImGui::Text(L("color"));
            ImGui::SameLine();
            if (ImGui::ColorEdit4("##TextColor", text.color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar))
            {
                if (syncAll && Settings::textObjects.size() > 1)
                {
                    for (auto& t : Settings::textObjects)
                    {
                        memcpy(t.color, text.color, sizeof(text.color));
                    }
                }
                Helpers::SaveCurrentPreset();
            }
            
            // Shadow
            if (ImGui::Checkbox(L("shadow"), &text.shadow))
            {
                if (syncAll && Settings::textObjects.size() > 1)
                {
                    for (auto& t : Settings::textObjects)
                    {
                        t.shadow = text.shadow;
                    }
                }
                Helpers::SaveCurrentPreset();
            }
        }
        else
        {
            ImGui::TextDisabled(L("click_plus_create"));
        }
        
        ImGui::EndChild();
        ImGui::SetCursorScreenPos(ImVec2(startPos.x, startPos.y + PANEL_HEIGHT + 5));
    }
}
