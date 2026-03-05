#include "pch.h"
#include "Layout.h"
#include "../../IMGUI/imgui_internal.h"
#include <commdlg.h>

namespace Layout
{
    static constexpr float SPLITTER_WIDTH = 8.0f;
    static constexpr float MIN_LEFT_WIDTH = 120.0f;
    static constexpr float MIN_RIGHT_WIDTH = 200.0f;
    
    float HandleSplitter(const char* id, float& leftWidth, ImVec2 startPos, float panelHeight, float availableWidth)
    {
        float rightWidth = availableWidth - leftWidth - SPLITTER_WIDTH;
        
        ImRect splitterRect(
            ImVec2(startPos.x + leftWidth, startPos.y),
            ImVec2(startPos.x + leftWidth + SPLITTER_WIDTH, startPos.y + panelHeight)
        );
        
        ImGui::SplitterBehavior(
            splitterRect,
            ImGui::GetID(id),
            ImGuiAxis_X,
            &leftWidth,
            &rightWidth,
            MIN_LEFT_WIDTH,
            MIN_RIGHT_WIDTH,
            0.0f
        );
        
        if (ImGui::IsItemHovered() || ImGui::IsItemActive())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }
        
        return rightWidth;
    }
    
    bool OpenFileDialog(char* outPath, size_t pathSize, const char* filter)
    {
        OPENFILENAMEA ofn = {};
        char buffer[512] = "";
        
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = buffer;
        ofn.nMaxFile = sizeof(buffer);
        ofn.lpstrFilter = filter;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
        
        if (!GetOpenFileNameA(&ofn))
        {
            return false;
        }
        
        strncpy(outPath, buffer, pathSize - 1);
        outPath[pathSize - 1] = '\0';
        return true;
    }
    
    void DrawSeparator()
    {
        ImGui::Spacing();
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        float width = ImGui::GetContentRegionAvail().x;
        ImGui::GetWindowDrawList()->AddLine(
            cursorPos,
            ImVec2(cursorPos.x + width, cursorPos.y),
            IM_COL32(100, 100, 100, 200),
            1.0f
        );
        ImGui::Dummy(ImVec2(0, 5));
    }
}
