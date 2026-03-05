#include "pch.h"
#include "FontSystem.h"
#include "../../resource.h"
#include "../../Utils/ResourceExtractor.h"
#include "../../UI/Overlay/Settings.h"
#include "../../Utils/Helpers.h"
#include <bakkesmod/wrappers/GuiManagerWrapper.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <algorithm>
#include <commdlg.h>
#include <codecvt>
#include <locale>

namespace FontSystem
{
    // ==================== State ====================

    struct EmbeddedFont
    {
        const char* displayName;
        const char* fileName;
        int resourceId;
    };

    static const EmbeddedFont EMBEDDED_FONTS[] = {
        {"Bourgeois Medium", "BourgeoisMedium.ttf", IDR_FONT_BOURGEOIS_MED},
        {"Bourgeois Thin", "BourgeoisThin.ttf", IDR_FONT_BOURGEOIS_THIN},
        {"Arial", "Arial.ttf", IDR_FONT_ARIAL},
        {"Arial Narrow", "ArialNarrow.ttf", IDR_FONT_ARIAL_NARROW},
        {"Arial Narrow Bold", "ArialNarrowBold.ttf", IDR_FONT_ARIAL_NARROW_B},
        {"Dashboard Numbers", "DashboardNumbers.ttf", IDR_FONT_DASHBOARD},
    };
    static const int EMBEDDED_FONT_COUNT = sizeof(EMBEDDED_FONTS) / sizeof(EMBEDDED_FONTS[0]);

    static std::shared_ptr<GameWrapper> gameWrapper;
    static std::string pluginName;
    static std::string fontsDirectory;
    static std::string presetFontsDirectory;
    static std::set<std::string> pendingFonts;
    static std::vector<std::string> customFontNames;

    // ==================== Lifecycle ====================

    void Initialize(std::shared_ptr<GameWrapper> wrapper, const std::string& pluginName)
    {
        FontSystem::gameWrapper = wrapper;
        FontSystem::pluginName = pluginName;
        fontsDirectory = wrapper->GetDataFolder().string() + "\\" + pluginName + "\\fonts";
        std::filesystem::create_directories(fontsDirectory);
    }

    void SetPresetFolder(const std::string& presetPath)
    {
        presetFontsDirectory = presetPath + "\\fonts";
        std::filesystem::create_directories(presetFontsDirectory);

        customFontNames.clear();
        for (const auto& entry : std::filesystem::directory_iterator(presetFontsDirectory))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            std::string ext = entry.path().extension().string();
            if (ext == ".ttf" || ext == ".otf")
            {
                customFontNames.push_back(entry.path().filename().string());
            }
        }
    }

    // ==================== Internal Helpers ====================

    static int FindEmbeddedFontIndex(const std::string& name)
    {
        for (int i = 0; i < EMBEDDED_FONT_COUNT; i++)
        {
            if (name == EMBEDDED_FONTS[i].displayName)
            {
                return i;
            }
        }
        return -1;
    }

    static void ExtractEmbeddedFont(int index)
    {
        std::string filePath = fontsDirectory + "\\" + EMBEDDED_FONTS[index].fileName;
        HMODULE hModule = GetCurrentModule();
        if (hModule)
        {
            ExtractResource(hModule, EMBEDDED_FONTS[index].resourceId, filePath);
        }
    }

    static void CheckPendingFonts()
    {
        for (auto it = pendingFonts.begin(); it != pendingFonts.end(); )
        {
            if (gameWrapper->GetGUIManager().GetFont(*it))
            {
                it = pendingFonts.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    // ==================== Public API ====================

    std::vector<std::string> GetFontNames()
    {
        std::vector<std::string> result;
        for (int i = 0; i < EMBEDDED_FONT_COUNT; i++)
        {
            result.push_back(EMBEDDED_FONTS[i].displayName);
        }
        return result;
    }

    const unsigned char* GetFontData(const std::string& name, unsigned int& outSize)
    {
        int index = FindEmbeddedFontIndex(name);
        if (index < 0 && EMBEDDED_FONT_COUNT > 0)
        {
            index = 0;
        }
        if (index < 0)
        {
            outSize = 0;
            return nullptr;
        }

        HMODULE hModule = GetCurrentModule();
        if (!hModule)
        {
            outSize = 0;
            return nullptr;
        }

        HRSRC hRes = FindResource(hModule, MAKEINTRESOURCE(EMBEDDED_FONTS[index].resourceId), RT_RCDATA);
        if (!hRes)
        {
            outSize = 0;
            return nullptr;
        }

        HGLOBAL hData = LoadResource(hModule, hRes);
        if (!hData)
        {
            outSize = 0;
            return nullptr;
        }

        outSize = SizeofResource(hModule, hRes);
        return (const unsigned char*)LockResource(hData);
    }

    ImFont* GetFont(const std::string& name, float targetSize)
    {
        if (name.empty())
        {
            return nullptr;
        }

        // Fixed base size (200px) - scaling done dynamically when rendering
        constexpr int BASE_SIZE = 200;
        int fontSize = BASE_SIZE;

        int embeddedIndex = FindEmbeddedFontIndex(name);
        if (embeddedIndex >= 0)
        {
            std::string fontId = pluginName + "_" + EMBEDDED_FONTS[embeddedIndex].fileName + "_" + std::to_string(fontSize);

            ImFont* font = gameWrapper->GetGUIManager().GetFont(fontId);
            if (font)
            {
                return font;
            }

            if (pendingFonts.find(fontId) == pendingFonts.end())
            {
                ExtractEmbeddedFont(embeddedIndex);

                // BakkesMod uses paths relative to bakkesmod/data/fonts/
                std::string relativePath = "../" + pluginName + "/fonts/" + EMBEDDED_FONTS[embeddedIndex].fileName;
                auto [result, loadedFont] = gameWrapper->GetGUIManager().LoadFont(fontId, relativePath, fontSize);

                // result == 2 means font is ready immediately
                if (result == 2 && loadedFont)
                {
                    return loadedFont;
                }

                // result == 1 means queued for loading
                pendingFonts.insert(fontId);
                gameWrapper->SetTimeout([](GameWrapper*) {
                    CheckPendingFonts();
                }, 1.0f);
            }

            return nullptr;
        }

        // Custom font (stored as full filename like "FontName.ttf")
        for (const auto& fontFile : customFontNames)
        {
            std::string displayName = std::filesystem::path(fontFile).stem().string();

            if (displayName != name)
            {
                continue;
            }

            std::string fontId = pluginName + "_custom_" + fontFile + "_" + std::to_string(fontSize);

            ImFont* font = gameWrapper->GetGUIManager().GetFont(fontId);
            if (font)
            {
                return font;
            }

            if (pendingFonts.find(fontId) == pendingFonts.end())
            {
                // Copy font to main fonts directory (BakkesMod LoadFont relative path)
                std::string sourcePath = presetFontsDirectory + "\\" + fontFile;
                std::string destPath = fontsDirectory + "\\" + fontFile;

                if (!std::filesystem::exists(destPath))
                {
                    if (!std::filesystem::exists(sourcePath))
                    {
                        return nullptr;
                    }

                    std::error_code ec;
                    std::filesystem::copy_file(sourcePath, destPath, ec);
                    if (ec)
                    {
                        return nullptr;
                    }
                }

                std::string relativePath = "../" + pluginName + "/fonts/" + fontFile;
                auto [result, loadedFont] = gameWrapper->GetGUIManager().LoadFont(fontId, relativePath, fontSize);

                if (result == 2 && loadedFont)
                {
                    return loadedFont;
                }

                if (result == 1)
                {
                    pendingFonts.insert(fontId);
                    gameWrapper->SetTimeout([](GameWrapper*) {
                        CheckPendingFonts();
                    }, 1.0f);
                }
            }

            return nullptr;
        }

        return nullptr;
    }

    std::string ImportFont()
    {
        char buffer[MAX_PATH] = "";
        OPENFILENAMEA ofn = {};

        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFilter = "Font Files\0*.ttf;*.otf\0";
        ofn.lpstrFile = buffer;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (!GetOpenFileNameA(&ofn))
        {
            return "";
        }

        std::filesystem::path sourcePath(buffer);

        if (!std::filesystem::exists(sourcePath))
        {
            return "";
        }

        std::string ext = sourcePath.extension().string();
        if (ext != ".ttf" && ext != ".otf")
        {
            return "";
        }

        std::string originalStem = sourcePath.stem().string();
        std::string sanitizedStem = Helpers::SanitizeFilename(originalStem, "font");
        std::string sanitizedFilename = sanitizedStem + ext;

        std::filesystem::path destPath = std::filesystem::path(presetFontsDirectory) / sanitizedFilename;

        try
        {
            std::filesystem::copy_file(
                sourcePath,
                destPath,
                std::filesystem::copy_options::overwrite_existing
            );
        }
        catch (const std::exception&)
        {
            return "";
        }

        if (!std::filesystem::exists(destPath))
        {
            return "";
        }

        // Add to custom fonts list if not already present
        bool found = false;
        for (const auto& existingFont : customFontNames)
        {
            if (existingFont == sanitizedFilename)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            customFontNames.push_back(sanitizedFilename);
        }

        return sanitizedStem;
    }

    bool RenderSelector(std::string& currentFont, const char* suffix)
    {
        if (currentFont.empty())
        {
            currentFont = EMBEDDED_FONTS[0].displayName;
        }

        bool changed = false;

        char comboId[64];
        snprintf(comboId, sizeof(comboId), "##FontCombo%s", suffix ? suffix : "");

        ImGui::SetNextItemWidth(150);
        if (ImGui::BeginCombo(comboId, currentFont.c_str()))
        {
            for (int i = 0; i < EMBEDDED_FONT_COUNT; i++)
            {
                bool isSelected = (currentFont == EMBEDDED_FONTS[i].displayName);
                if (ImGui::Selectable(EMBEDDED_FONTS[i].displayName, isSelected))
                {
                    currentFont = EMBEDDED_FONTS[i].displayName;
                    changed = true;
                }
            }

            if (!customFontNames.empty())
            {
                ImGui::Separator();

                for (const auto& fontFile : customFontNames)
                {
                    std::string displayName = std::filesystem::path(fontFile).stem().string();
                    bool isSelected = (currentFont == displayName);
                    if (ImGui::Selectable(displayName.c_str(), isSelected))
                    {
                        currentFont = displayName;
                        changed = true;
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::SameLine();

        char buttonId[48];
        snprintf(buttonId, sizeof(buttonId), "Importar##Font%s", suffix ? suffix : "");

        if (ImGui::Button(buttonId))
        {
            std::string imported = ImportFont();
            if (!imported.empty())
            {
                currentFont = imported;
                changed = true;
            }
        }

        return changed;
    }
}
