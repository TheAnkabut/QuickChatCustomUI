#include "pch.h"
#include "QuickChatCustomUI.h"
#include "Overlay/Media.h"
#include "Overlay/Text.h"
#include "RL/BlockUI.h"
#include "RL/QuickChatConfig.h"
#include "UI/Overlay/Settings.h"
#include "Overlay/Fonts/FontSystem.h"
#include "Persistence/Presets.h"
#include "Persistence/DirectoryWatcher.h"
#include "Utils/Localization.h"

BAKKESMOD_PLUGIN(QuickChatCustomUI, "Quick Chat Custom UI", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

static const char* PLUGIN_NAME = "QuickChatCustomUI";

void QuickChatCustomUI::onLoad()
{
    _globalCvarManager = cvarManager;

    SendCount();

    std::string dataFolder = gameWrapper->GetDataFolder().string() + "\\" + PLUGIN_NAME;
    std::filesystem::create_directories(dataFolder);

    Localization::Initialize(gameWrapper);
    FontSystem::Initialize(gameWrapper, PLUGIN_NAME);
    Media::Initialize(gameWrapper);
    Presets::Initialize(dataFolder);
    Settings::Initialize(gameWrapper);
    BlockUI::Initialize(gameWrapper);
    QuickChatConfig::Initialize(gameWrapper);
}

void QuickChatCustomUI::onUnload()
{
    DirectoryWatcher::Stop();
    BlockUI::Shutdown();
}

void QuickChatCustomUI::RenderSettings()
{
    // Discord link
    ImVec4 discordColor = ImVec4(0.30f, 0.85f, 1.0f, 1.0f);
    ImGui::TextColored(discordColor, L("join_discord"));
    if (ImGui::IsItemHovered())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddLine(ImVec2(min.x, max.y), ImVec2(max.x, max.y), ImGui::GetColorU32(discordColor), 1.0f);
    }
    if (ImGui::IsItemClicked())
    {
        ShellExecuteA(nullptr, "open", "https://discord.gg/FPvkjaBPEA", nullptr, nullptr, SW_SHOWNORMAL);
    }
    ImGui::SameLine();
    ImGui::Text("·");
    ImGui::SameLine();
    ImGui::Text("Made by The Ankabut");
    ImGui::Spacing();

    Settings::Render();
}

void QuickChatCustomUI::Render()
{
    if (!Settings::showOverlay)
    {
        return;
    }

    Media::RenderAll();
    Text::Render();
}

// ==================== Count ====================

void QuickChatCustomUI::SendCount()
{
    cvarManager->registerCvar("customui_counted", "0", "Count flag", true, true, 0, true, 1, true);

    gameWrapper->SetTimeout([this](GameWrapper* gw) {
        auto sentCvar = cvarManager->getCvar("customui_counted");
        if (!sentCvar || sentCvar.getIntValue() == 1)
        {
            LOG("[Count] Already sent, skipping.");
            return;
        }

        // Player ID
        std::string idRL;
        if (gameWrapper->IsUsingEpicVersion()) {
            idRL = gameWrapper->GetEpicID();
        } else {
            idRL = std::to_string(gameWrapper->GetSteamID());
        }

        // Player name
        std::string playerName = gameWrapper->GetPlayerName().ToString();

        // Request body
        std::string body = "{\"pluginName\":\"Quick Chat Custom UI\",\"idRL\":\"" + idRL + "\",\"playerName\":\"" + playerName + "\"}";

        CurlRequest req;
        req.url = "https://script.google.com/macros/s/AKfycbyudCL3iGwkhkoCGVDpIL7FOOkXrgTkDjd8g6yJtIz7qwiQ11IOoebbwD5YFL4ctayU/exec";
        req.body = body;

        HttpWrapper::SendCurlJsonRequest(req, [this](int code, std::string result) {
            if (code == 0 || result == "error") {
                LOG("[Count] Failed, will retry next load.");
                return;
            }
            auto sentCvar = cvarManager->getCvar("customui_counted");
            if (sentCvar) {
                sentCvar.setValue(1);
                cvarManager->executeCommand("writeconfig", false);
                LOG("[Count] Sent successfully. Total: {}", result);
            }
        });
    }, 9.0f);
}
