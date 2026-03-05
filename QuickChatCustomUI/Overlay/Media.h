#pragma once
#include "../UI/Overlay/Settings.h"
#include <memory>

namespace Media
{
    void Initialize(std::shared_ptr<GameWrapper> wrapper);

    void LoadImage(const std::string& path);
    void LoadGif(const std::string& path);
    void LoadGifFrames(const std::string& framesDir);

    void Render(Settings::MediaObject& mediaObj);
    void RenderAll();
}
