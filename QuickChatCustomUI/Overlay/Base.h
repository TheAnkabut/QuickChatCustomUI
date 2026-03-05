#pragma once
#include <algorithm>
#include "../UI/Overlay/Settings.h"

namespace OverlayBase
{
    inline float& GlobalOffsetX()
    {
        static float val = 0.0f;
        return val;
    }

    inline float& GlobalOffsetY()
    {
        static float val = 0.0f;
        return val;
    }

    inline float& GlobalScale()
    {
        static float val = 1.0f;
        return val;
    }

    inline ImVec2 CalculatePosition(float offsetX, float offsetY, ImVec2 screenSize, float width = 0.0f, float height = 0.0f)
    {
        float scale = GlobalScale();

        // Position as percentage of screen
        float x = (offsetX / 100.0f) * screenSize.x;
        float y = (offsetY / 100.0f) * screenSize.y;

        // Apply global offset
        x += (GlobalOffsetX() / 100.0f) * screenSize.x;
        y += (GlobalOffsetY() / 100.0f) * screenSize.y;

        // Scale relative to screen center so all elements move as a group
        float centerX = screenSize.x * 0.5f;
        float centerY = screenSize.y * 0.5f;
        x = centerX + (x - centerX) * scale;
        y = centerY + (y - centerY) * scale;

        (void)width;
        (void)height;

        return ImVec2(x, y);
    }

    namespace Fade
    {
        // Perceptual gamma values for smooth fade transition
        constexpr float TEXT_GAMMA_BASE = 2.2f;
        constexpr float TEXT_GAMMA_BRIGHT = 1.0f;
        constexpr float MEDIA_GAMMA = 2.2f;

        inline float Text(float r, float g, float b, float progress)
        {
            if (Settings::fadeMode == Settings::FadeMode::LINEAR)
            {
                return progress;
            }

            float brightness = fmaxf(fmaxf(r, g), b);
            float gamma = TEXT_GAMMA_BASE + (brightness * TEXT_GAMMA_BRIGHT);
            return powf(progress, gamma);
        }

        inline float Media(float progress)
        {
            if (Settings::fadeMode == Settings::FadeMode::LINEAR)
            {
                return progress;
            }

            return powf(progress, MEDIA_GAMMA);
        }
    }
}
