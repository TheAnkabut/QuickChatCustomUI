#pragma once
#include <array>
#include <memory>
#include <string>
#include <vector>

namespace Settings
{
    void Initialize(std::shared_ptr<GameWrapper> wrapper);
    void Render();

    extern bool showOverlay;
    extern float opacity;

    extern float globalOffsetX;
    extern float globalOffsetY;
    extern float globalScale;

    // Fade
    enum class FadeMode { LINEAR, SMOOTH };
    extern FadeMode fadeMode;

    extern float fadeInDuration;
    extern float fadeOutSlow;
    extern float fadeOutFast;

    struct MediaObject
    {
        enum Type { IMAGE, GIF } type = IMAGE;
        char path[512] = "";
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        float scale = 1.0f;
        float opacity = 100.0f;
        float roundCorners = 0.0f;
        int speed = 100;
        uint8_t groupMask = 0; // 0=all, bit0=G1..bit4=G5

        // Runtime (not persisted)
        int currentFrame = 0;
        float timeAccumulator = 0.0f;
    };

    extern std::vector<MediaObject> mediaObjects;
    extern int selectedMediaIndex;

    struct TextObject
    {
        char content[256] = "";
        std::string fontName = "Bourgeois Medium";
        float offsetX = 50.0f;
        float offsetY = 50.0f;
        float fontSize = 48.0f;
        float opacity = 100.0f;
        float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        bool shadow = false;
        uint8_t groupMask = 0; // 0=all, bit0=G1..bit4=G5
    };

    extern std::vector<TextObject> textObjects;
    extern int selectedTextIndex;

    struct QuickChatSlot
    {
        char customText[256] = "";
        std::string fontName = "Bourgeois Medium";
        float offsetX = 50.0f;
        float offsetY = 20.0f;
        float fontSize = 32.0f;
        float opacity = 100.0f;
        // RL default blue: 0x43B0FF
        float color[4] = {0.263f, 0.690f, 1.0f, 1.0f};
        // RL selected: white
        float selectedColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        bool shadow = false;
        float shadowColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    };

    extern std::array<QuickChatSlot, 20> quickChatSlots;
    extern int selectedQCSlotIndex;
    extern int activeQCGroup;
    extern int boldSlotIndex;

    // Per-group CustomUI toggle (false = disabled, RL handles it)
    extern bool enabledGroups[5];

    void SetActiveQCGroup(int group);
    void SetBoldSlot(int slot);
}
