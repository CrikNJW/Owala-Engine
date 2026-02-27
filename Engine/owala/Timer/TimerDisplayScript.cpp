#include "pch.h"
#include "TimerDisplayScript.h"
#include "ScriptRegistry.h"
#include "TimerSystem.h"
#include "EntityManager.h"
#include "GameManager.h"

REGISTER_SCRIPT(TimerDisplayScript, "TimerDisplayScript")

// ---------------------------------------------------------------------------
void TimerDisplayScript::OnCreate() {
    // Nothing needed - we draw directly every frame
}

// ---------------------------------------------------------------------------
void TimerDisplayScript::OnUpdate() {
    GameManager& gm = GameManager::GetInstance();
    Graphics* gfx = gm.GetSystems().gfx_;
    ResourceManager* rcm = gm.GetSystems().resourceManager_;
    CameraSystem* camera = gm.GetSystems().camera_;

    if (!gfx || !rcm || !camera) return;

    Font* font = nullptr;
    try {
        font = &rcm->get<Font>(m_fontKey);
    }
    catch (...) { return; }

    std::string display = TimerSystem::Get().GetDisplayString();

    // Get the active camera for transformations
    Entity activeCam = camera->GetActiveCamera();

    // Your position is in NDC already, so no world-to-screen conversion needed
    Vector2D screenPos = m_position;

    // But you might need to apply the same zoom correction as SpriteRenderSystem
    if (activeCam != 0) {
        const Camera& cam = camera->GetCamera(activeCam);
        float zoomRatio = 350.0f / cam.zoom;  // Same as in CollectText
        m_scale *= zoomRatio;
    }

    // IMPORTANT: Draw at the right time - you need to hook into the 
    // SpriteRenderSystem's rendering order or render after it
    // For now, just draw and hope it's not overwritten
    gfx->setAlphaBlend(true);
    gfx->drawText(*font, display, screenPos, m_scale, m_color);
    gfx->setAlphaBlend(false);
}
// ---------------------------------------------------------------------------
void TimerDisplayScript::OnDestroy() {}