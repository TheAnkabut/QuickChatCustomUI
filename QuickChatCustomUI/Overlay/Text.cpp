#include "pch.h"
#include "Text.h"
#include "Base.h"
#include "../UI/Overlay/Settings.h"
#include "../RL/BlockUI.h"
#include "../RL/QuickChatConfig.h"
#include "Fonts/FontSystem.h"

namespace Text
{
    static int previewGroup = 0;
    
    void SetPreviewGroup(int group)
    {
        previewGroup = (group >= 0 && group < 5) ? group : 0;
    }
    
    static void DrawText(ImDrawList* drawList, ImFont* font, float fontSize, ImVec2 position, ImU32 color, const char* text, bool shadow, float alpha, const float* shadowColor = nullptr)
    {
        // Shadow (2px offset, reduced alpha)
        if (shadow)
        {
            ImU32 shadowCol;
            if (shadowColor)
            {
                shadowCol = IM_COL32(
                    (int)(shadowColor[0] * 255),
                    (int)(shadowColor[1] * 255),
                    (int)(shadowColor[2] * 255),
                    (int)(alpha * shadowColor[3] * 0.8f * 255)
                );
            }
            else
            {
                shadowCol = IM_COL32(0, 0, 0, (int)(alpha * 0.8f * 255));
            }
            drawList->AddText(font, fontSize, ImVec2(position.x + 2, position.y + 2), shadowCol, text);
        }
        
        drawList->AddText(font, fontSize, position, color, text);
    }
    
    void Render()
    {
        if (!Settings::showOverlay)
        {
            return;
        }
        
        float fadeProgress = BlockUI::GetFadeOpacity();
        
        ImVec2 screenSize = ImGui::GetIO().DisplaySize;
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        float globalScale = OverlayBase::GlobalScale();
        
        for (auto& textObj : Settings::textObjects)
        {
            if (textObj.content[0] == '\0')
            {
                continue;
            }
            
            // Skip if group filter active and not matching
            if (textObj.groupMask != 0 && Settings::activeQCGroup >= 0 && Settings::activeQCGroup < 5)
            {
                if (!(textObj.groupMask & (1 << Settings::activeQCGroup)))
                {
                    continue;
                }
            }
            
            ImFont* font = FontSystem::GetFont(textObj.fontName, textObj.fontSize);
            if (!font)
            {
                font = ImGui::GetFont();
            }
            
            // Scale font by global scale, not the font's base size
            float fontSize = textObj.fontSize * globalScale;
            
            ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0, textObj.content);
            ImVec2 position = OverlayBase::CalculatePosition(textObj.offsetX, textObj.offsetY, screenSize, textSize.x, textSize.y);
            
            float effectiveFade = OverlayBase::Fade::Text(textObj.color[0], textObj.color[1], textObj.color[2], fadeProgress);
            float alpha = (textObj.opacity / 100.0f) * effectiveFade * textObj.color[3];
            ImU32 color = IM_COL32(
                (int)(textObj.color[0] * 255), 
                (int)(textObj.color[1] * 255), 
                (int)(textObj.color[2] * 255), 
                (int)(alpha * 255)
            );
            
            DrawText(drawList, font, fontSize, position, color, textObj.content, textObj.shadow, alpha);
        }
        
        // Determine active QuickChat group (use preview group in BakkesMod UI)
        int activeGroup = Settings::activeQCGroup;
        if (activeGroup < 0 && fadeProgress >= 0.99f && QuickChatConfig::IsLoaded())
        {
            activeGroup = previewGroup;
        }
        
        if (activeGroup < 0 || activeGroup > 4 || !QuickChatConfig::IsLoaded())
        {
            return;
        }
        
        const auto& bindings = QuickChatConfig::GetBindings();
        
        for (int i = 0; i < 4; i++)
        {
            int slotIndex = activeGroup * 4 + i;
            int bindingIndex = activeGroup * 4 + i;
            
            if (bindingIndex >= 24)
            {
                continue;
            }
            
            auto& slot = Settings::quickChatSlots[slotIndex];
            const auto& originalMessage = bindings[bindingIndex].message;
            
            // Custom text overrides the original QC message
            std::string displayText = (slot.customText[0] != '\0') ? slot.customText : originalMessage;
            
            if (displayText.empty())
            {
                continue;
            }
            
            ImFont* font = FontSystem::GetFont(slot.fontName, slot.fontSize);
            if (!font)
            {
                font = ImGui::GetFont();
            }
            
            // Scale font by global scale
            float fontSize = slot.fontSize * globalScale;
            
            ImVec2 position = OverlayBase::CalculatePosition(slot.offsetX, slot.offsetY, screenSize, 0, 0);
            bool isSelected = (slotIndex == Settings::boldSlotIndex);
            const float* activeColor = isSelected ? slot.selectedColor : slot.color;
            
            float effectiveFade = OverlayBase::Fade::Text(activeColor[0], activeColor[1], activeColor[2], fadeProgress);
            float alpha = (slot.opacity / 100.0f) * effectiveFade * activeColor[3];
            ImU32 color = IM_COL32(
                (int)(activeColor[0] * 255), 
                (int)(activeColor[1] * 255), 
                (int)(activeColor[2] * 255), 
                (int)(alpha * 255)
            );
            
            DrawText(drawList, font, fontSize, position, color, displayText.c_str(), slot.shadow, alpha, slot.shadowColor);
        }
    }
}
