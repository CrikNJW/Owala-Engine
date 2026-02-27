#pragma once
#include "ScriptableEntity.h"
#include "Graphics.h"

using namespace Framework;

// ---------------------------------------------------------------------------
// TimerDisplayScript
//
//  Draws the timer directly via Graphics::drawText every frame.
//  This bypasses the ECS text rendering pipeline entirely, so the
//  entity does NOT need TextComponent, GUITag, LoadedTag, etc.
//
//  Setup:
//    1. Create any empty entity in your level scene
//    2. Add NativeScriptsComponent and attach TimerDisplayScript
//    3. Set m_fontKey below to match your loaded font key
//    4. That's it - no other components needed
// ---------------------------------------------------------------------------
class TimerDisplayScript : public ScriptableEntity {
protected:
    void OnCreate()  override;
    void OnUpdate()  override;
    void OnDestroy() override;

private:
    // ---- CONFIGURE THESE ----
    // Must match the key used when loading the font in ResourceManager
    // e.g. resources->Load<Font>("default", "Assets/Fonts/arial.ttf")
    std::string m_fontKey = "default";

    // NDC position: {0,0} = center, {-1,1} = top-left, {-0.1, 0.9} = top-center
    Vector2D    m_position = { -0.1f, 0.9f };

    // Text scale (same as TextComponent::scale)
    float       m_scale = 1.0f;

    // Text color RGBA
    Color       m_color = { 1.0f, 1.0f, 0.0f, 1.0f }; // bright yellow
};