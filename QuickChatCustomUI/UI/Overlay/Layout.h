#pragma once
#include "Settings.h"

namespace Layout
{
    float HandleSplitter(const char* id, float& leftWidth, ImVec2 startPos, 
                         float panelHeight, float availableWidth);
    
    bool OpenFileDialog(char* outPath, size_t pathSize, const char* filter);
    void DrawSeparator();
}
