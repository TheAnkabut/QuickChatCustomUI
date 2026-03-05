#include "pch.h"
#include "Panels.h"
#include "../Settings.h"
#include "../Layout.h"
#include "../../../Overlay/Media.h"
#include "../../../Utils/Helpers.h"
#include "../../../IMGUI/imgui_internal.h"
#include "../../../Utils/Localization.h"
#include <algorithm>
#include <filesystem>

namespace MediaPanel
{
    static float splitterWidth = 180.0f;
    static constexpr float PANEL_HEIGHT = 200.0f;
    static bool syncAll = false;

    static std::string ExtractFilename(const std::string& path)
    {
        return std::filesystem::path(path).filename().string();
    }
    
    void Render()
    {
        ImGui::Separator();
        if (!ImGui::CollapsingHeader(L("multimedia_manager")))
        {
            return;
        }
        
        float availableWidth = ImGui::GetContentRegionAvail().x;
        ImVec2 startPos = ImGui::GetCursorScreenPos();
        float rightWidth = Layout::HandleSplitter("##MediaSplitter", splitterWidth, startPos, PANEL_HEIGHT, availableWidth);
        
        // Left panel: media list
        ImGui::SetCursorScreenPos(ImVec2(startPos.x + 7, startPos.y));
        ImGui::BeginChild("##MediaLeftPanel", ImVec2(splitterWidth - 7, PANEL_HEIGHT), false);
        
        // Add button
        if (ImGui::Button("+##AddMedia"))
        {
            Settings::mediaObjects.push_back(Settings::MediaObject());
            Settings::selectedMediaIndex = (int)Settings::mediaObjects.size() - 1;
            Helpers::SaveCurrentPreset();
        }
        
        ImGui::SameLine();
        
        // Remove button
        if (ImGui::Button("-##RemoveMedia") && !Settings::mediaObjects.empty() && Settings::selectedMediaIndex >= 0)
        {
            Settings::mediaObjects.erase(Settings::mediaObjects.begin() + Settings::selectedMediaIndex);
            if (Settings::selectedMediaIndex >= (int)Settings::mediaObjects.size())
            {
                Settings::selectedMediaIndex = (int)Settings::mediaObjects.size() - 1;
            }
            Helpers::SaveCurrentPreset();
        }
        
        // Media list
        ImGui::BeginChild("##MediaList", ImVec2(0, 0), true);
        for (int i = 0; i < (int)Settings::mediaObjects.size(); i++)
        {
            Settings::MediaObject& media = Settings::mediaObjects[i];
            
            // Determine type prefix
            std::string typePrefix = "[?] ";
            if (media.path[0] != '\0')
            {
                typePrefix = (media.type == Settings::MediaObject::GIF) ? "[G] " : "[I] ";
            }
            
            std::string filename = media.path[0] ? ExtractFilename(media.path) : L("no_file");
            std::string label = typePrefix + filename + "##media" + std::to_string(i);
            
            if (ImGui::Selectable(label.c_str(), Settings::selectedMediaIndex == i))
            {
                Settings::selectedMediaIndex = i;
            }
        }
        ImGui::EndChild();
        ImGui::EndChild();
        
        // Right panel: properties
        ImGui::SetCursorScreenPos(ImVec2(startPos.x + splitterWidth + 8 + 7, startPos.y));
        ImGui::BeginChild("##MediaRightPanel", ImVec2(rightWidth, PANEL_HEIGHT), false);
        
        if (Settings::selectedMediaIndex >= 0 && Settings::selectedMediaIndex < (int)Settings::mediaObjects.size())
        {
            Settings::MediaObject& media = Settings::mediaObjects[Settings::selectedMediaIndex];
            
            // Display filename
            std::string displayName = media.path[0] ? ExtractFilename(media.path) : L("none");
            ImGui::Text("%s", displayName.c_str());
            ImGui::SameLine();
            
            // Import button
            if (ImGui::Button(L("import")))
            {
                char tempPath[512] = "";
                if (Layout::OpenFileDialog(tempPath, sizeof(tempPath), "Media\0*.png;*.jpg;*.gif\0"))
                {
                    std::string ext = std::filesystem::path(tempPath).extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    bool isGif = (ext == ".gif");
                    
                    // Import with sanitized filename
                    std::string destPath = Presets::ImportMediaFile(tempPath);
                    if (!destPath.empty())
                    {
                        strncpy(media.path, destPath.c_str(), sizeof(media.path) - 1);
                        media.type = isGif ? Settings::MediaObject::GIF : Settings::MediaObject::IMAGE;
                        Helpers::SaveCurrentPreset();
                        
                        if (media.type == Settings::MediaObject::IMAGE)
                        {
                            Media::LoadImage(destPath);
                        }
                        else
                        {
                            Media::LoadGif(destPath);
                        }
                    }
                }
            }
            
            // Sync checkbox
            ImGui::Checkbox(L("sync"), &syncAll);
            
            // Position
            float position[2] = {media.offsetX, media.offsetY};
            if (ImGui::DragFloat2(L("position"), position, 0.5f, -1000.0f, 1000.0f, "%.1f%%"))
            {
                float deltaX = position[0] - media.offsetX;
                float deltaY = position[1] - media.offsetY;
                
                if (syncAll && Settings::mediaObjects.size() > 1)
                {
                    for (auto& m : Settings::mediaObjects)
                    {
                        m.offsetX += deltaX;
                        m.offsetY += deltaY;
                    }
                }
                else
                {
                    media.offsetX = position[0];
                    media.offsetY = position[1];
                }
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                Helpers::SaveCurrentPreset();
            }
            
            // Scale
            if (ImGui::SliderFloat(L("scale"), &media.scale, 0.01f, 5.0f, "%.2fx"))
            {
                if (syncAll && Settings::mediaObjects.size() > 1)
                {
                    for (auto& m : Settings::mediaObjects)
                    {
                        m.scale = media.scale;
                    }
                }
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                Helpers::SaveCurrentPreset();
            }
            
            // Opacity
            if (ImGui::SliderFloat(L("opacity"), &media.opacity, 0.0f, 100.0f, "%.0f%%"))
            {
                if (syncAll && Settings::mediaObjects.size() > 1)
                {
                    for (auto& m : Settings::mediaObjects)
                    {
                        m.opacity = media.opacity;
                    }
                }
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                Helpers::SaveCurrentPreset();
            }
            
            // Round corners 
            ImGui::SliderFloat(L("roundness"), &media.roundCorners, 0.0f, 2.0f, "%.2f");
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                Helpers::SaveCurrentPreset();
            }
            
            // GIF speed 
            if (media.type == Settings::MediaObject::GIF)
            {
                ImGui::SliderInt(L("speed"), &media.speed, 10, 500, "%dms");
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    Helpers::SaveCurrentPreset();
                }
            }
            
            // Group selector with checkboxes (multi-select)
            ImGui::Text(L("qc_group"));
            ImGui::SameLine();
            for (int g = 0; g < 5; g++)
            {
                bool checked = (media.groupMask & (1 << g)) != 0;
                char label[8];
                sprintf(label, "%d##mg%d", g + 1, g);
                if (ImGui::Checkbox(label, &checked))
                {
                    if (checked)
                    {
                        media.groupMask |= (1 << g);
                    }
                    else
                    {
                        media.groupMask &= ~(1 << g);
                    }

                    if (syncAll && Settings::mediaObjects.size() > 1)
                    {
                        for (auto& m : Settings::mediaObjects)
                        {
                            m.groupMask = media.groupMask;
                        }
                    }
                    Helpers::SaveCurrentPreset();
                }
                if (g < 3)
                {
                    ImGui::SameLine();
                }
            }
            // Show "all" label if no groups selected
            if (media.groupMask == 0)
            {
                ImGui::SameLine();
                ImGui::TextDisabled("(%s)", L("all"));
            }
        }
        else
        {
            ImGui::TextDisabled(L("no_object_selected"));
        }
        
        ImGui::EndChild();
        ImGui::SetCursorScreenPos(ImVec2(startPos.x, startPos.y + PANEL_HEIGHT + 5));
    }
}
