#include "pch.h"
#include "TimerSystem.h"
#include "FirebaseManager.h"

namespace Framework {

    // ---------------------------------------------------------------------------
    void TimerSystem::StartTimer() {
        m_elapsed = 0.0f;
        m_running = true;
        m_finished = false;
        m_waitingForMove = false;
    }

    // ---------------------------------------------------------------------------
    // Arms the timer so it starts the moment the player moves.
    // Call this from wherever you want movement-triggered start behaviour
    // (e.g. from a level-load event, or from TimerStartTrigger if you still
    //  want a start zone but prefer movement-detection inside it).
    void TimerSystem::NotifyPlayerMoved() {
        if (m_running || m_finished) return;   // already going / already done

        // Auto-start on first movement
        m_elapsed = 0.0f;
        m_running = true;
        m_finished = false;
        m_waitingForMove = false;
    }

    // ---------------------------------------------------------------------------
    void TimerSystem::StopAndSubmit() {
        if (!m_running) return;
        m_running = false;
        m_finished = true;

        float finalTime = m_elapsed;
        FirebaseManager::Get().SubmitRun(finalTime);
    }

    // ---------------------------------------------------------------------------
    void TimerSystem::Reset() {
        m_running = false;
        m_finished = false;
        m_waitingForMove = false;
        m_elapsed = 0.0f;
    }

    // ---------------------------------------------------------------------------
    void TimerSystem::Update(float dt) {
        if (m_running)
            m_elapsed += dt;
    }

} // namespace Framework