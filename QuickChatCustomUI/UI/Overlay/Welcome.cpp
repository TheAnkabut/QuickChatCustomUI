#include "pch.h"
#include "Welcome.h"
#include "../../Persistence/Presets.h"
#include "../../Utils/Localization.h"

namespace Welcome
{
    static char firstPresetName[128] = "";
    static bool focusSet = false;
    
    void Render()
    {
        float windowWidth = ImGui::GetContentRegionAvail().x;
        float windowHeight = ImGui::GetContentRegionAvail().y;
        
        float contentHeight = 120.0f;
        ImGui::SetCursorPosY((windowHeight - contentHeight) * 0.5f);
        
        // Title
        ImVec4 green(0.4f, 1.0f, 0.4f, 1.0f);
        const char* title = "Quick Chat Custom UI";
        float titleWidth = ImGui::CalcTextSize(title).x;
        ImGui::SetCursorPosX((windowWidth - titleWidth) * 0.5f);
        ImGui::TextColored(green, "%s", title);
        
        ImGui::Spacing();
        ImGui::Spacing();
        
        // Label
        const char* label = L("create_first_preset");
        float labelWidth = ImGui::CalcTextSize(label).x;
        ImGui::SetCursorPosX((windowWidth - labelWidth) * 0.5f);
        ImGui::TextUnformatted(label);
        
        ImGui::Spacing();
        
        float inputWidth = 150.0f;
        float buttonWidth = 60.0f;
        float totalWidth = inputWidth + 10.0f + buttonWidth;
        ImGui::SetCursorPosX((windowWidth - totalWidth) * 0.5f);
        
        if (!focusSet)
        {
            ImGui::SetKeyboardFocusHere();
            focusSet = true;
        }
        
        ImGui::SetNextItemWidth(inputWidth);
        bool enterPressed = ImGui::InputText("##WelcomePreset", firstPresetName, 128, 
                                              ImGuiInputTextFlags_EnterReturnsTrue);
        
        ImGui::SameLine();
        
        bool canCreate = firstPresetName[0] != '\0';
        
        if (!canCreate)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.3f);
        }
        
        bool createClicked = ImGui::Button(L("create"), ImVec2(buttonWidth, 0));
        
        if (!canCreate)
        {
            ImGui::PopStyleVar();
        }
        
        if (canCreate && (createClicked || enterPressed))
        {
            if (Presets::CreatePreset(firstPresetName))
            {
                firstPresetName[0] = '\0';
                focusSet = false;
            }
        }
    }
}
