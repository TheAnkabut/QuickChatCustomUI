#pragma once
#include <string>
#include "../Persistence/Presets.h"

namespace Helpers
{
    // Sanitizes a filename for safe filesystem use.
    inline std::string SanitizeFilename(const std::string& filename, const std::string& fallback = "file")
    {
        std::string result;
        result.reserve(filename.size());

        for (unsigned char c : filename)
        {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-')
            {
                result += c;
            }
            else if (c == ' ' || c > 127)
            {
                result += '_';
            }
        }

        // Remove consecutive underscores
        std::string cleaned;
        bool lastWasUnderscore = false;
        for (char c : result)
        {
            if (c == '_')
            {
                if (!lastWasUnderscore)
                {
                    cleaned += c;
                    lastWasUnderscore = true;
                }
            }
            else
            {
                cleaned += c;
                lastWasUnderscore = false;
            }
        }

        size_t start = cleaned.find_first_not_of("_ ");
        size_t end = cleaned.find_last_not_of("_ ");
        if (start == std::string::npos)
        {
            return fallback;
        }

        return cleaned.substr(start, end - start + 1);
    }

    // Saves the current preset (used by panels)
    inline void SaveCurrentPreset()
    {
        std::string presetName = Presets::GetCurrentPresetName();
        if (!presetName.empty())
        {
            Presets::SavePreset(presetName);
        }
    }
} 
