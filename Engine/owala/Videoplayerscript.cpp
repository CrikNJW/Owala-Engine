#include "pch.h"
#include "VideoPlayerScript.h"
#include "ScriptRegistry.h"
#include "GameManager.h"
#include "Graphics.h"
#include "Logger.h"

REGISTER_SCRIPT(VideoPlayerScript, "VideoPlayerScript")

void VideoPlayerScript::OnCreate() {
    m_player.SetLooping(loop);

    // Add a safety check: If the path is just "video.mp4", prepend the folder
    std::string finalPath = videoPath;
    if (finalPath.find("/") == std::string::npos && finalPath.find("\\") == std::string::npos) {
        finalPath = "Assets/Sprites/Cutscenes/" + videoPath;
    }

    Logger::Get().Info(DebugType::GameManager)
        << "[VideoPlayerScript] Final Path Resolution: " << finalPath << std::endl;

    if (!m_player.Open(finalPath)) {
        Logger::Get().Error(DebugType::GameManager)
            << "[VideoPlayerScript] Failed to open: " << finalPath << std::endl;
        return;
    }

    if (autoPlay) m_player.Play();
}

void VideoPlayerScript::OnUpdate() {
    if (!m_player.IsReady()) {
        static int logCount = 0;
        logCount++;
        if (logCount % 60 == 0) {  // Log every 60 frames
            Logger::Get().Info(DebugType::GameManager)
                << "[VideoPlayerScript] Player not ready yet" << std::endl;
        }
        return;
    }

    m_player.Update(Framework::Time::DeltaTime());
    DrawFullscreen();

    // Log playback status occasionally
    static int frameCount = 0;
    frameCount++;
    if (frameCount % 60 == 0) {
        Logger::Get().Info(DebugType::GameManager)
            << "[VideoPlayerScript] Playing - Position: "
            << m_player.GetPosition() << "/" << m_player.GetDuration()
            << " State: " << (int)m_player.GetState() << std::endl;
    }
}

// FIX: Added the missing OnDestroy method to resolve LNK2001
void VideoPlayerScript::OnDestroy() {
    Logger::Get().Info(DebugType::GameManager)
        << "[VideoPlayerScript] Destroying player" << std::endl;
    m_player.Close();
}

void VideoPlayerScript::DrawFullscreen() {
    if (!m_player.IsReady()) return;

    Graphics* gfx = GameManager::GetInstance().GetSystems().gfx_;
    if (!gfx) return;

    GLuint texID = m_player.GetTexture();
    if (texID == 0) return;

    Framework::Texture tex = static_cast<Framework::Texture>(texID);

    // center={0,0} = NDC center (correct)
    // half={16/9, 1} = drawSprite applies aspectCorrection internally:
    // effective X = (16/9) * (1080/1920) = (16/9) * (9/16) = 1.0 ? fullscreen
    Vector2D center = { 0.0f, 0.0f };
    Vector2D half = { 16.0f / 9.0f, 1.0f };

    // Disable blend so video is fully opaque — no transparency
    gfx->setAlphaBlend(false);
    gfx->drawSprite(tex, center, half, 0.0f, { 0.0f, 0.0f }, { 1.0f, 1.0f });
    glDisable(GL_BLEND);  // drawSprite re-enables blend internally, force it off after
    gfx->setAlphaBlend(true);
}