#include "pch.h"
#include "Media.h"
#include "Base.h"
#include "../RL/BlockUI.h"
#include "../Utils/Helpers.h"
#include <filesystem>
#include <fstream>
#include <map>

#define STB_IMAGE_IMPLEMENTATION
#include "../Utils/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../Utils/stb_image_write.h"

namespace Media
{
    // imgui_draw.cpp lines 992-1010 modified to properly handle rounded
    // corners on images via AddImageRounded

    // ==================== State ====================

    struct GifData
    {
        std::vector<std::shared_ptr<ImageWrapper>> frames;
        std::vector<float> durations;
        bool loaded = false;
    };

    static std::shared_ptr<GameWrapper> gameWrapper;
    static std::map<std::string, std::shared_ptr<ImageWrapper>> imageCache;
    static std::map<std::string, std::shared_ptr<GifData>> gifCache;

    // ==================== Lifecycle ====================

    void Initialize(std::shared_ptr<GameWrapper> wrapper)
    {
        Media::gameWrapper = wrapper;
    }

    // ==================== Loading ====================

    void LoadImage(const std::string& path)
    {
        if (path.empty() || imageCache.count(path) || !std::filesystem::exists(path))
        {
            return;
        }

        std::string imagePath = path;
        gameWrapper->SetTimeout([imagePath](GameWrapper*) {
            if (!imageCache.count(imagePath) && std::filesystem::exists(imagePath))
            {
                imageCache[imagePath] = std::make_shared<ImageWrapper>(imagePath, true, true);
            }
        }, 0.01f);
    }

    void LoadGif(const std::string& path)
    {
        if (path.empty() || gifCache.count(path) || !std::filesystem::exists(path))
        {
            return;
        }

        std::string gifPath = path;
        gameWrapper->SetTimeout([gifPath](GameWrapper*) {
            if (gifCache.count(gifPath))
            {
                return;
            }

            auto gifData = std::make_shared<GifData>();
            gifCache[gifPath] = gifData;

            // Frames directory: preset/media/frames/sanitizedBaseName/
            std::filesystem::path gifFilePath(gifPath);
            std::filesystem::path mediaDir = gifFilePath.parent_path();
            std::string baseName = Helpers::SanitizeFilename(gifFilePath.stem().string(), "gif");
            std::filesystem::path framesDir = mediaDir / "frames" / baseName;
            std::filesystem::create_directories(framesDir);

            // Check if frames are already cached on disk
            std::string firstFrame = (framesDir / "frame_0.png").string();
            if (std::filesystem::exists(firstFrame))
            {
                for (int i = 0; ; i++)
                {
                    std::string framePath = (framesDir / ("frame_" + std::to_string(i) + ".png")).string();
                    if (!std::filesystem::exists(framePath))
                    {
                        break;
                    }

                    gifData->frames.push_back(std::make_shared<ImageWrapper>(framePath, true, true));
                    gifData->durations.push_back(0.05f);
                }

                gifData->loaded = !gifData->frames.empty();
                return;
            }

            // Decode GIF from file
            std::ifstream file(gifPath, std::ios::binary | std::ios::ate);
            if (!file)
            {
                return;
            }

            size_t fileSize = (size_t)file.tellg();
            std::vector<unsigned char> fileData(fileSize);
            file.seekg(0);
            file.read((char*)fileData.data(), fileSize);
            file.close();

            int* frameDelays = nullptr;
            int width, height, frameCount, channels;
            unsigned char* pixelData = stbi_load_gif_from_memory(
                fileData.data(),
                (int)fileSize,
                &frameDelays,
                &width,
                &height,
                &frameCount,
                &channels,
                4
            );

            if (!pixelData || frameCount == 0)
            {
                stbi_image_free(pixelData);
                return;
            }

            // Extract and save each frame as PNG
            int bytesPerFrame = width * height * 4;
            for (int i = 0; i < frameCount; i++)
            {
                std::string framePath = (framesDir / ("frame_" + std::to_string(i) + ".png")).string();
                if (stbi_write_png(framePath.c_str(), width, height, 4, pixelData + i * bytesPerFrame, width * 4))
                {
                    gifData->frames.push_back(std::make_shared<ImageWrapper>(framePath, true, true));
                    float duration = (frameDelays && frameDelays[i] > 0) ? frameDelays[i] / 1000.0f : 0.05f;
                    gifData->durations.push_back(duration);
                }
            }

            stbi_image_free(pixelData);
            free(frameDelays);

            gifData->loaded = !gifData->frames.empty();
        }, 0.01f);
    }

    void LoadGifFrames(const std::string& framesDir)
    {
        if (framesDir.empty() || gifCache.count(framesDir))
        {
            return;
        }

        std::string firstFrame = framesDir + "\\frame_0.png";
        if (!std::filesystem::exists(firstFrame))
        {
            return;
        }

        auto gifData = std::make_shared<GifData>();
        gifCache[framesDir] = gifData;

        for (int i = 0; ; i++)
        {
            std::string framePath = framesDir + "\\frame_" + std::to_string(i) + ".png";
            if (!std::filesystem::exists(framePath))
            {
                break;
            }

            gifData->frames.push_back(std::make_shared<ImageWrapper>(framePath, true, true));
            gifData->durations.push_back(0.05f);
        }

        gifData->loaded = !gifData->frames.empty();
    }

    // ==================== Rendering ====================

    static void DrawImage(ImDrawList* drawList, void* texture, ImVec2 position, float width, float height, float radius, ImU32 tint)
    {
        drawList->AddImageRounded(
            (ImTextureID)texture,
            position,
            ImVec2(position.x + width, position.y + height),
            ImVec2(0, 0),
            ImVec2(1, 1),
            tint,
            radius
        );
    }

    void Render(Settings::MediaObject& mediaObj)
    {
        if (mediaObj.path[0] == '\0')
        {
            return;
        }

        ImVec2 screenSize = ImGui::GetIO().DisplaySize;
        float baseScale = (screenSize.y / 1080.0f) * mediaObj.scale * OverlayBase::GlobalScale();

        float fadeProgress = BlockUI::GetFadeOpacity();
        float effectiveFade = OverlayBase::Fade::Media(fadeProgress);
        float alpha = (mediaObj.opacity / 100.0f) * effectiveFade;
        ImU32 tintColor = IM_COL32(255, 255, 255, (int)(alpha * 255));
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        void* texture = nullptr;
        float width = 0;
        float height = 0;

        if (mediaObj.type == Settings::MediaObject::IMAGE)
        {
            auto it = imageCache.find(mediaObj.path);
            if (it == imageCache.end())
            {
                LoadImage(mediaObj.path);
                return;
            }

            auto& image = it->second;
            if (!image->IsLoadedForImGui())
            {
                image->LoadForImGui([](bool){});
                return;
            }

            texture = image->GetImGuiTex();
            if (!texture)
            {
                return;
            }

            Vector2 size = image->GetSize();
            width = size.X * baseScale;
            height = size.Y * baseScale;
        }
        else
        {
            auto it = gifCache.find(mediaObj.path);
            if (it == gifCache.end() || !it->second->loaded)
            {
                std::string framesCheck = std::string(mediaObj.path) + "\\frame_0.png";
                if (std::filesystem::exists(framesCheck))
                {
                    LoadGifFrames(mediaObj.path);
                }
                else if (std::filesystem::exists(mediaObj.path))
                {
                    LoadGif(mediaObj.path);
                }
                return;
            }

            auto& gifData = it->second;

            // GIF animation timing
            mediaObj.timeAccumulator += 1.0f / ImGui::GetIO().Framerate;
            float speedMultiplier = (mediaObj.speed > 0) ? mediaObj.speed / 100.0f : 1.0f;

            if (mediaObj.currentFrame >= (int)gifData->frames.size())
            {
                mediaObj.currentFrame = 0;
            }

            if (mediaObj.timeAccumulator >= gifData->durations[mediaObj.currentFrame] / speedMultiplier)
            {
                mediaObj.timeAccumulator = 0;
                mediaObj.currentFrame = (mediaObj.currentFrame + 1) % (int)gifData->frames.size();
            }

            auto& frame = gifData->frames[mediaObj.currentFrame];
            if (!frame->IsLoadedForImGui())
            {
                frame->LoadForImGui([](bool){});
                return;
            }

            texture = frame->GetImGuiTex();
            if (!texture)
            {
                return;
            }

            Vector2 size = frame->GetSize();
            width = size.X * baseScale;
            height = size.Y * baseScale;
        }

        ImVec2 position = OverlayBase::CalculatePosition(mediaObj.offsetX, mediaObj.offsetY, screenSize, width, height);
        float radius = mediaObj.roundCorners * (std::min)(width, height) * 0.5f;
        DrawImage(drawList, texture, position, width, height, radius, tintColor);
    }

    void RenderAll()
    {
        int activeGroup = Settings::activeQCGroup;

        for (auto& mediaObj : Settings::mediaObjects)
        {
            // groupMask == 0 means visible in all groups
            if (mediaObj.groupMask != 0 && activeGroup >= 0 && activeGroup < 5)
            {
                if (!(mediaObj.groupMask & (1 << activeGroup)))
                {
                    continue;
                }
            }
            Render(mediaObj);
        }
    }
}
