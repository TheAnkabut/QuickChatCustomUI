#pragma once
#include <string>
#include <vector>
#include <memory>

struct ImFont;

namespace FontSystem
{
    void Initialize(std::shared_ptr<GameWrapper> wrapper, const std::string& pluginName);
    void SetPresetFolder(const std::string& presetPath);

    // targetSize picks the best-sized font for crisp rendering
    ImFont* GetFont(const std::string& name = "", float targetSize = 48.0f);
    std::vector<std::string> GetFontNames();

    // Raw font data for software rendering (stb_truetype)
    const unsigned char* GetFontData(const std::string& name, unsigned int& outSize);

    std::string ImportFont();
    bool RenderSelector(std::string& currentFont, const char* suffix = "");
}
