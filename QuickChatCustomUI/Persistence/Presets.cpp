#include "pch.h"
#include "Presets.h"
#include "../UI/Overlay/Settings.h"
#include "../Overlay/Media.h"
#include "../Overlay/Fonts/FontSystem.h"
#include "../RL/QuickChatConfig.h"
#include "../RL/BlockUI.h"
#include "../Utils/Helpers.h"
#include "DirectoryWatcher.h"

namespace Presets
{
    // ==================== State ====================

    static std::string dataFolder;
    static std::string presetsFolder;
    static std::string currentPresetName;

    // User config values (persisted in user_config.json)
    static float configDuration = 1.5f;
    static std::string configActivePreset;

    // ==================== Lifecycle ====================

    void Initialize(const std::string& folder)
    {
        dataFolder = folder;
        presetsFolder = folder + "\\Presets";
        std::filesystem::create_directories(presetsFolder);

        // Wire QuickChatConfig -> Presets: auto-save when bindings update from game memory
        QuickChatConfig::SetOnBindingsUpdated([]() {
            SaveUserConfig();
        });

        // Load user config (cached bindings, duration, active preset)
        LoadUserConfig();

        std::string savedPreset = GetConfigActivePreset();
        if (!savedPreset.empty())
        {
            LoadPreset(savedPreset);
        }
        else if (!GetPresetList().empty())
        {
            LoadPreset(GetPresetList()[0]);
        }

        if (configDuration > 0)
        {
            BlockUI::SetDisplayDuration(configDuration);
        }
    }

    // ==================== Internal Helpers ====================

    static std::string GetPresetPath(const std::string& name)
    {
        return presetsFolder + "\\" + name;
    }

    static std::string GetPresetJsonPath(const std::string& name)
    {
        return GetPresetPath(name) + "\\preset.json";
    }

    static std::string GetUserConfigPath()
    {
        return dataFolder + "\\user_config.json";
    }

    // ==================== Preset Discovery ====================

    std::string GetPresetsFolder()
    {
        return presetsFolder;
    }

    std::string GetCurrentPresetName()
    {
        return currentPresetName;
    }

    void SetCurrentPresetName(const std::string& name)
    {
        currentPresetName = name;
    }

    std::vector<std::string> GetPresetList()
    {
        std::vector<std::string> result;
        std::error_code ec;

        for (const auto& entry : std::filesystem::directory_iterator(presetsFolder, ec))
        {
            if (entry.is_directory())
            {
                result.push_back(entry.path().filename().string());
            }
        }

        return result;
    }

    // ==================== Preset Operations ====================

    bool CreatePreset(const std::string& name)
    {
        if (name.empty())
        {
            return false;
        }

        std::error_code ec;
        std::filesystem::create_directories(GetPresetPath(name), ec);
        if (ec)
        {
            return false;
        }

        Settings::mediaObjects.clear();
        Settings::textObjects.clear();
        Settings::selectedMediaIndex = -1;
        Settings::selectedTextIndex = -1;

        for (int i = 0; i < 20; i++)
        {
            Settings::quickChatSlots[i] = Settings::QuickChatSlot();
            Settings::quickChatSlots[i].offsetX = 50.0f;
            Settings::quickChatSlots[i].offsetY = 20.0f + (i % 4) * 15.0f;
        }

        currentPresetName = name;
        FontSystem::SetPresetFolder(GetPresetPath(name));
        return SavePreset(name);
    }

    bool RenamePreset(const std::string& oldName, const std::string& newName)
    {
        if (oldName.empty() || newName.empty() || oldName == newName)
        {
            return false;
        }

        std::string oldPath = GetPresetPath(oldName);
        std::string newPath = GetPresetPath(newName);

        if (!std::filesystem::exists(oldPath) || std::filesystem::exists(newPath))
        {
            return false;
        }

        std::error_code ec;
        std::filesystem::rename(oldPath, newPath, ec);
        if (ec)
        {
            return false;
        }

        if (currentPresetName == oldName)
        {
            currentPresetName = newName;
        }

        return true;
    }

    bool DeletePreset(const std::string& name)
    {
        if (name.empty()) return false;

        std::string path = GetPresetPath(name);
        if (!std::filesystem::exists(path)) return false;

        std::error_code ec;
        std::filesystem::remove_all(path, ec);
        if (ec) return false;

        if (currentPresetName == name)
        {
            currentPresetName.clear();
        }

        return true;
    }

    std::string ImportMediaFile(const std::string& sourcePath)
    {
        if (sourcePath.empty() || currentPresetName.empty())
        {
            return "";
        }

        if (!std::filesystem::exists(sourcePath))
        {
            return "";
        }

        std::string presetPath = GetPresetPath(currentPresetName);
        std::string mediaPath = presetPath + "\\media";
        std::filesystem::create_directories(mediaPath);

        std::string originalFilename = std::filesystem::path(sourcePath).filename().string();
        std::string sanitizedFilename = Helpers::SanitizeFilename(originalFilename, "media");
        std::string destPath = mediaPath + "\\" + sanitizedFilename;

        if (sourcePath != destPath)
        {
            std::error_code ec;
            std::filesystem::copy_file(sourcePath, destPath, std::filesystem::copy_options::overwrite_existing, ec);
            if (ec)
            {
                return "";
            }
        }

        return destPath;
    }

    // ==================== Save / Load Preset ====================

    bool SavePreset(const std::string& name)
    {
        if (name.empty())
        {
            return false;
        }

        DirectoryWatcher::IgnoreNextChange();

        std::string presetPath = GetPresetPath(name);
        std::string mediaPath = presetPath + "\\media";
        std::filesystem::create_directories(mediaPath);

        nlohmann::ordered_json root;
        root["name"] = name;
        root["version"] = 1;

        // Media objects
        root["media"] = nlohmann::json::array();
        for (auto& mediaObj : Settings::mediaObjects)
        {
            nlohmann::json mediaJson;
            mediaJson["type"] = (mediaObj.type == Settings::MediaObject::GIF) ? "GIF" : "IMAGE";

            std::string sourcePath = mediaObj.path;
            std::string relativePath;

            if (!sourcePath.empty() && std::filesystem::exists(sourcePath))
            {
                std::string baseName = std::filesystem::path(sourcePath).stem().string();

                if (mediaObj.type == Settings::MediaObject::IMAGE)
                {
                    std::string originalFilename = std::filesystem::path(sourcePath).filename().string();
                    std::string sanitizedFilename = Helpers::SanitizeFilename(originalFilename, "media");
                    std::string destPath = mediaPath + "\\" + sanitizedFilename;

                    if (sourcePath != destPath)
                    {
                        std::error_code ec;
                        std::filesystem::copy_file(sourcePath, destPath, std::filesystem::copy_options::overwrite_existing, ec);
                    }

                    relativePath = "media\\" + sanitizedFilename;
                    strncpy(mediaObj.path, destPath.c_str(), sizeof(mediaObj.path) - 1);
                }
                else
                {
                    // GIF: frames stored in media/frames/sanitizedBaseName/
                    std::string sanitizedBaseName = Helpers::SanitizeFilename(baseName, "gif");
                    std::string framesPath = mediaPath + "\\frames\\" + sanitizedBaseName + "\\frame_0.png";

                    if (std::filesystem::exists(framesPath))
                    {
                        relativePath = "media\\frames\\" + sanitizedBaseName;
                    }
                    else
                    {
                        std::string originalFilename = std::filesystem::path(sourcePath).filename().string();
                        std::string sanitizedFilename = Helpers::SanitizeFilename(originalFilename, "media");
                        std::string destPath = mediaPath + "\\" + sanitizedFilename;

                        if (sourcePath != destPath)
                        {
                            std::error_code ec;
                            std::filesystem::copy_file(sourcePath, destPath, std::filesystem::copy_options::overwrite_existing, ec);
                        }

                        relativePath = "media\\" + sanitizedFilename;
                        strncpy(mediaObj.path, destPath.c_str(), sizeof(mediaObj.path) - 1);
                    }
                }
            }

            mediaJson["path"] = relativePath;
            mediaJson["offsetX"] = mediaObj.offsetX;
            mediaJson["offsetY"] = mediaObj.offsetY;
            mediaJson["scale"] = mediaObj.scale;
            mediaJson["opacity"] = mediaObj.opacity;
            mediaJson["speed"] = mediaObj.speed;
            mediaJson["roundCorners"] = mediaObj.roundCorners;
            mediaJson["groupMask"] = mediaObj.groupMask;
            root["media"].push_back(mediaJson);
        }

        // Text objects
        root["text"] = nlohmann::json::array();
        for (const auto& textObj : Settings::textObjects)
        {
            nlohmann::json textJson;
            textJson["content"] = textObj.content;
            textJson["fontName"] = textObj.fontName;
            textJson["fontSize"] = textObj.fontSize;
            textJson["offsetX"] = textObj.offsetX;
            textJson["offsetY"] = textObj.offsetY;
            textJson["color"] = {textObj.color[0], textObj.color[1], textObj.color[2], textObj.color[3]};
            textJson["opacity"] = textObj.opacity;
            textJson["shadow"] = textObj.shadow;
            textJson["groupMask"] = textObj.groupMask;
            root["text"].push_back(textJson);
        }

        // QuickChat slots
        root["quickchats"] = nlohmann::json::array();
        for (const auto& slot : Settings::quickChatSlots)
        {
            nlohmann::json slotJson;
            slotJson["customText"] = slot.customText;
            slotJson["fontName"] = slot.fontName;
            slotJson["fontSize"] = slot.fontSize;
            slotJson["offsetX"] = slot.offsetX;
            slotJson["offsetY"] = slot.offsetY;
            slotJson["color"] = {slot.color[0], slot.color[1], slot.color[2], slot.color[3]};
            slotJson["selectedColor"] = {slot.selectedColor[0], slot.selectedColor[1], slot.selectedColor[2], slot.selectedColor[3]};
            slotJson["opacity"] = slot.opacity;
            slotJson["shadow"] = slot.shadow;
            slotJson["shadowColor"] = {slot.shadowColor[0], slot.shadowColor[1], slot.shadowColor[2], slot.shadowColor[3]};
            root["quickchats"].push_back(slotJson);
        }

        // Enabled groups (per-group CustomUI toggle)
        root["enabledGroups"] = nlohmann::json::array();
        for (int i = 0; i < 5; i++)
        {
            root["enabledGroups"].push_back(Settings::enabledGroups[i]);
        }

        std::ofstream file(GetPresetJsonPath(name));
        if (!file)
        {
            return false;
        }

        // error_handler_t::replace handles non-UTF8 characters in paths
        file << root.dump(2, ' ', false, nlohmann::json::error_handler_t::replace);
        return true;
    }

    bool LoadPreset(const std::string& name)
    {
        if (name.empty())
        {
            return false;
        }

        std::string jsonPath = GetPresetJsonPath(name);
        if (!std::filesystem::exists(jsonPath))
        {
            return false;
        }

        std::ifstream file(jsonPath);
        if (!file)
        {
            return false;
        }

        nlohmann::json root;
        try
        {
            root = nlohmann::json::parse(file);
        }
        catch (...)
        {
            return false;
        }

        std::string presetPath = GetPresetPath(name);

        Settings::mediaObjects.clear();
        Settings::textObjects.clear();
        Settings::selectedMediaIndex = -1;
        Settings::selectedTextIndex = -1;
        FontSystem::SetPresetFolder(presetPath);

        // Media objects
        if (root.contains("media"))
        {
            for (const auto& mediaJson : root["media"])
            {
                Settings::MediaObject mediaObj;
                mediaObj.type = (mediaJson.value("type", "IMAGE") == "GIF")
                    ? Settings::MediaObject::GIF
                    : Settings::MediaObject::IMAGE;

                std::string relativePath = mediaJson.value("path", "");
                std::string absolutePath = relativePath.empty() ? "" : presetPath + "\\" + relativePath;
                strncpy(mediaObj.path, absolutePath.c_str(), sizeof(mediaObj.path) - 1);

                mediaObj.offsetX = mediaJson.value("offsetX", 0.0f);
                mediaObj.offsetY = mediaJson.value("offsetY", 0.0f);
                mediaObj.scale = mediaJson.value("scale", 1.0f);
                mediaObj.opacity = mediaJson.value("opacity", 100.0f);
                mediaObj.speed = mediaJson.value("speed", 100);
                mediaObj.roundCorners = mediaJson.value("roundCorners", 0.0f);
                mediaObj.groupMask = mediaJson.value("groupMask", (uint8_t)0);

                Settings::mediaObjects.push_back(mediaObj);

                if (!absolutePath.empty())
                {
                    if (mediaObj.type == Settings::MediaObject::IMAGE)
                    {
                        if (std::filesystem::exists(absolutePath))
                        {
                            Media::LoadImage(absolutePath);
                        }
                    }
                    else
                    {
                        // GIF: path can be a frames folder or a .gif file
                        std::string framesCheck = absolutePath + "\\frame_0.png";
                        if (std::filesystem::exists(framesCheck))
                        {
                            Media::LoadGifFrames(absolutePath);
                        }
                        else if (std::filesystem::exists(absolutePath))
                        {
                            Media::LoadGif(absolutePath);
                        }
                    }
                }
            }
        }

        // Text objects
        if (root.contains("text"))
        {
            for (const auto& textJson : root["text"])
            {
                Settings::TextObject textObj;
                strncpy(textObj.content, textJson.value("content", "").c_str(), sizeof(textObj.content) - 1);
                textObj.fontName = textJson.value("fontName", "Bourgeois Medium");
                textObj.fontSize = textJson.value("fontSize", 48.0f);
                textObj.offsetX = textJson.value("offsetX", 50.0f);
                textObj.offsetY = textJson.value("offsetY", 50.0f);
                textObj.opacity = textJson.value("opacity", 100.0f);
                textObj.shadow = textJson.value("shadow", true);
                textObj.groupMask = textJson.value("groupMask", (uint8_t)0);

                if (textJson.contains("color") && textJson["color"].size() >= 4)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        textObj.color[i] = textJson["color"][i];
                    }
                }

                Settings::textObjects.push_back(textObj);
            }
        }

        // QuickChat slots
        if (root.contains("quickchats"))
        {
            int index = 0;
            for (const auto& slotJson : root["quickchats"])
            {
                if (index >= 20)
                {
                    break;
                }

                auto& slot = Settings::quickChatSlots[index];

                std::string customStr = slotJson.value("customText", "");
                strncpy(slot.customText, customStr.c_str(), sizeof(slot.customText) - 1);
                slot.customText[sizeof(slot.customText) - 1] = '\0';

                slot.fontName = slotJson.value("fontName", "Bourgeois Medium");
                slot.fontSize = slotJson.value("fontSize", 32.0f);
                slot.offsetX = slotJson.value("offsetX", 50.0f);
                slot.offsetY = slotJson.value("offsetY", 20.0f + (index % 4) * 15.0f);
                slot.opacity = slotJson.value("opacity", 100.0f);
                slot.shadow = slotJson.value("shadow", true);

                if (slotJson.contains("color") && slotJson["color"].size() >= 4)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        slot.color[i] = slotJson["color"][i];
                    }
                }

                if (slotJson.contains("selectedColor") && slotJson["selectedColor"].size() >= 4)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        slot.selectedColor[i] = slotJson["selectedColor"][i];
                    }
                }

                if (slotJson.contains("shadowColor") && slotJson["shadowColor"].size() >= 4)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        slot.shadowColor[i] = slotJson["shadowColor"][i];
                    }
                }

                index++;
            }
        }

        // Enabled groups (per-group CustomUI toggle)
        if (root.contains("enabledGroups"))
        {
            int enabledCount = (std::min)((int)root["enabledGroups"].size(), 5);
            for (int i = 0; i < enabledCount; i++)
            {
                Settings::enabledGroups[i] = root["enabledGroups"][i].get<bool>();
            }
        }

        currentPresetName = name;
        return true;
    }

    // ==================== User Config ====================

    void LoadUserConfig()
    {
        std::string configPath = GetUserConfigPath();
        if (!std::filesystem::exists(configPath))
        {
            return;
        }

        std::ifstream file(configPath);
        if (!file)
        {
            return;
        }

        nlohmann::json root;
        try
        {
            root = nlohmann::json::parse(file);
        }
        catch (...)
        {
            return;
        }

        configDuration = root.value("duration", 1.5f);
        configActivePreset = root.value("activePreset", "");

        // Load cached bindings into QuickChatConfig
        if (root.contains("bindings") && root["bindings"].is_array())
        {
            auto& bindings = QuickChatConfig::GetMutableBindings();
            int index = 0;
            for (const auto& bindingJson : root["bindings"])
            {
                if (index >= 24)
                {
                    break;
                }

                bindings[index].messageId = bindingJson.value("messageId", "");
                bindings[index].message = bindingJson.value("message", "");
                bindings[index].group = bindingJson.value("group", 0);
                index++;
            }

            if (index > 0)
            {
                QuickChatConfig::SetLoaded(true);
            }
        }
    }

    void SaveUserConfig()
    {
        nlohmann::ordered_json root;
        root["duration"] = configDuration;
        root["activePreset"] = configActivePreset;

        // Cache bindings from QuickChatConfig
        const auto& bindings = QuickChatConfig::GetBindings();
        root["bindings"] = nlohmann::json::array();
        for (const auto& binding : bindings)
        {
            nlohmann::json bindingJson;
            bindingJson["messageId"] = binding.messageId;
            bindingJson["message"] = binding.message;
            bindingJson["group"] = binding.group;
            root["bindings"].push_back(bindingJson);
        }

        std::ofstream file(GetUserConfigPath());
        if (file)
        {
            file << root.dump(2, ' ', false, nlohmann::json::error_handler_t::replace);
        }
    }

    float GetConfigDuration()
    {
        return configDuration;
    }

    void SetConfigDuration(float duration)
    {
        configDuration = duration;
        SaveUserConfig();
    }

    std::string GetConfigActivePreset()
    {
        return configActivePreset;
    }

    void SetConfigActivePreset(const std::string& preset)
    {
        configActivePreset = preset;
        SaveUserConfig();
    }
}
