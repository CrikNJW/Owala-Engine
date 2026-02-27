#include "pch.h"
#include "TimerStartTrigger.h"
#include "ScriptRegistry.h"
#include "TimerSystem.h"
#include "AABB_Collision.h"
#include "EntityManager.h"
#include "GameManager.h"

REGISTER_SCRIPT(TimerStartTriggerScript, "TimerStartTriggerScript")

// ---------------------------------------------------------------------------
void TimerStartTriggerScript::OnCreate() {
    m_fired = false;
    m_inZone = false;
    m_lastPos = { 0.0f, 0.0f };
    m_gotFirstPos = false;
}

// ---------------------------------------------------------------------------
void TimerStartTriggerScript::OnUpdate() {
    if (m_fired) return;

    // Don't re-arm if the timer already finished (e.g. level restart edge case)
    if (TimerSystem::Get().IsFinished()) return;

    EntityStorage& ecs = GameManager::GetInstance().GetECS();
    EntityManager  mgr(ecs);
    Entity         self = GetEntity();

    // --- Find the player ---
    std::vector<Entity> players = mgr.GetAllEntitiesWithComponent(false, PlayerTagType);
    if (players.empty()) return;
    Entity player = players[0];

    if (!mgr.HasComponent(player, TransformType)) return;
    Transform& playerTf = mgr.GetComponent<Transform>(player);

    // --- Optional zone gate: only watch for movement after entering this zone ---
    if constexpr (USE_ZONE_GATE) {
        if (!m_inZone) {
            if (!mgr.HasComponent(player, AABBColliderType)) return;
            if (!mgr.HasComponent(self, AABBColliderType)) return;
            if (!mgr.HasComponent(self, TransformType))    return;

            AABBCollider& playerCol = mgr.GetComponent<AABBCollider>(player);
            AABBCollider& triggerCol = mgr.GetComponent<AABBCollider>(self);
            Transform& selfTf = mgr.GetComponent<Transform>(self);

            AABB_Bounds playerBounds{
                { playerTf.position.x - playerCol.HalfSize.x + playerCol.Offset.x,
                  playerTf.position.y - playerCol.HalfSize.y + playerCol.Offset.y },
                { playerTf.position.x + playerCol.HalfSize.x + playerCol.Offset.x,
                  playerTf.position.y + playerCol.HalfSize.y + playerCol.Offset.y }
            };
            AABB_Bounds triggerBounds{
                { selfTf.position.x - triggerCol.HalfSize.x + triggerCol.Offset.x,
                  selfTf.position.y - triggerCol.HalfSize.y + triggerCol.Offset.y },
                { selfTf.position.x + triggerCol.HalfSize.x + triggerCol.Offset.x,
                  selfTf.position.y + triggerCol.HalfSize.y + triggerCol.Offset.y }
            };

            if (!CollisionIntersection_RectRect_Static(playerBounds, triggerBounds))
                return; // not in zone yet

            m_inZone = true;
        }
    }

    // --- Detect movement ---
    // Priority 1: Velocity component (Velocity::Vel) - cleanest signal
    bool isMoving = false;

    if (mgr.HasComponent(player, VelocityType)) {
        Velocity& vel = mgr.GetComponent<Velocity>(player);
        constexpr float kVelThresholdSq = 1.0f * 1.0f; // 1 unit/s — tweak if needed
        float speedSq = vel.Vel.x * vel.Vel.x + vel.Vel.y * vel.Vel.y;
        isMoving = (speedSq > kVelThresholdSq);
    }
    else {
        // Priority 2: Positional delta fallback (no Velocity component present)
        if (!m_gotFirstPos) {
            // Record spawn position on the very first frame; don't fire yet
            m_lastPos = playerTf.position;
            m_gotFirstPos = true;
            return;
        }

        constexpr float kPosDeltaThresholdSq = 0.01f * 0.01f;
        float dx = playerTf.position.x - m_lastPos.x;
        float dy = playerTf.position.y - m_lastPos.y;
        isMoving = (dx * dx + dy * dy) > kPosDeltaThresholdSq;
        m_lastPos = playerTf.position;
    }

    if (isMoving) {
        TimerSystem::Get().NotifyPlayerMoved();
        m_fired = true;
    }
}

// ---------------------------------------------------------------------------
void TimerStartTriggerScript::OnDestroy() {}