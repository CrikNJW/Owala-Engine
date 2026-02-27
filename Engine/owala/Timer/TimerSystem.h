#pragma once
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace Framework {

    // ---------------------------------------------------------------------------
    // TimerSystem
    //
    //  Three ways the timer can start:
    //    1. Explicit call to StartTimer()          (original behaviour)
    //    2. Call to NotifyPlayerMoved()            (new – triggers on first input)
    //    3. TimerStartTrigger zone overlap         (original behaviour, uses StartTimer)
    //
    //  The timer stops (and submits to Firebase) when StopAndSubmit() is called,
    //  which the TimerFinishTrigger (portal) invokes.
    // ---------------------------------------------------------------------------
    class TimerSystem {
    public:
        static TimerSystem& Get() {
            static TimerSystem instance;
            return instance;
        }

        // ---- control ----
        void StartTimer();          // start / restart immediately
        void StopAndSubmit();       // stop + push time to Firebase
        void Reset();               // reset to 0, not running

        // Call this every frame from your player movement code (or PlayerSystem).
        // The timer will auto-start the very first frame the player moves.
        void NotifyPlayerMoved();

        // Call every frame from the main game loop to accumulate time.
        void Update(float dt);

        // ---- queries ----
        bool  IsRunning()   const { return m_running; }
        bool  IsFinished()  const { return m_finished; }
        float GetElapsed()  const { return m_elapsed; }

        // Returns a nicely formatted string: "MM:SS.mmm"
        std::string GetDisplayString() const {
            int totalMs = static_cast<int>(m_elapsed * 1000.0f);
            int ms = totalMs % 1000;
            int totalSec = totalMs / 1000;
            int sec = totalSec % 60;
            int min = totalSec / 60;

            std::ostringstream oss;
            oss << std::setfill('0')
                << std::setw(2) << min << ":"
                << std::setw(2) << sec << "."
                << std::setw(3) << ms;
            return oss.str();
        }

    private:
        TimerSystem() = default;

        bool  m_running = false;
        bool  m_finished = false;  // true after StopAndSubmit
        bool  m_waitingForMove = false;  // armed and waiting for first movement
        float m_elapsed = 0.0f;
    };

} // namespace Framework