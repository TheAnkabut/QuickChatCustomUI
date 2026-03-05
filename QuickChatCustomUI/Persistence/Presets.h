#pragma once
#include <string>
#include <vector>

namespace Presets
{
    void Initialize(const std::string& dataFolder);

    std::vector<std::string> GetPresetList();
    std::string GetPresetsFolder();

    std::string GetCurrentPresetName();
    void SetCurrentPresetName(const std::string& name);

    bool CreatePreset(const std::string& name);
    bool LoadPreset(const std::string& name);
    bool SavePreset(const std::string& name);
    bool RenamePreset(const std::string& oldName, const std::string& newName);
    bool DeletePreset(const std::string& name);

    // Sanitizes filename and copies to preset folder
    std::string ImportMediaFile(const std::string& sourcePath);

    // User config persistence (duration, active preset, bindings cache)
    void LoadUserConfig();
    void SaveUserConfig();
    float GetConfigDuration();
    void SetConfigDuration(float duration);
    std::string GetConfigActivePreset();
    void SetConfigActivePreset(const std::string& preset);
}
