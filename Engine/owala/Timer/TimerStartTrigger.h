#pragma once
#include "ScriptableEntity.h"
#include "Vector2D.h"

using namespace Framework;

class TimerStartTriggerScript : public ScriptableEntity {
protected:
    void OnCreate()  override;
    void OnUpdate()  override;
    void OnDestroy() override;

public:
    // Called by LevelResetSystem::RunReset() on player respawn so the
    // trigger re-arms and waits for the next movement input.
    void ResetFired() {
        m_fired = false;
        m_inZone = false;
        m_gotFirstPos = false;
        m_lastPos = { 0.0f, 0.0f };
    }

private:
    // Set true to only start watching for movement after the player
    // enters this trigger's zone. false = watch anywhere in the level.
    static constexpr bool USE_ZONE_GATE = false;

    bool     m_fired = false;
    bool     m_inZone = false;
    Vector2D m_lastPos = { 0.0f, 0.0f };
    bool     m_gotFirstPos = false;
};