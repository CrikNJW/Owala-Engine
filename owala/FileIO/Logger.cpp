/*****************************************************************//**
 * \file   Logger.cpp
 *
 * \primary_author
 *      Yar Xuan
 * \secondary_author
 *
 *
 * \brief
 *      This file contains the core components for engine-wide logging and error reporting.
 *      It provides a `GameException` struct that automatically captures a stack trace
 *      for easier debugging, and a singleton `Logger` class. The logger can output
 *      categorized messages to the console and create timestamped crash log files.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "pch.h"
#include "Logger.h"
#include <deque>
#include <memory>
#include <mutex>

namespace Framework {
    namespace {
        constexpr std::size_t MAX_BUFFERED_LOG_LINES = 2000;

        std::mutex gLogBufferMutex;
        std::deque<std::string> gBufferedLogLines;

        void AppendBufferedLine(std::string line) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.empty()) {
                return;
            }

            std::lock_guard<std::mutex> lock(gLogBufferMutex);
            gBufferedLogLines.push_back(std::move(line));
            if (gBufferedLogLines.size() > MAX_BUFFERED_LOG_LINES) {
                gBufferedLogLines.pop_front();
            }
        }

        std::once_flag gLogCaptureInitOnce;

        class TeeCaptureBuf final : public std::streambuf {
        public:
            explicit TeeCaptureBuf(std::streambuf* target) : target_(target) {}
            void FlushPendingLine() {
                std::lock_guard<std::mutex> lock(lineMutex_);
                FlushPendingLineUnlocked();
            }

        protected:
            int overflow(int ch) override {
                std::lock_guard<std::mutex> lock(lineMutex_);
                if (ch == EOF) {
                    return std::char_traits<char>::not_eof(ch);
                }

                const char c = static_cast<char>(ch);
                if (c == '\n') {
                    AppendBufferedLine(currentLine_);
                    currentLine_.clear();
                }
                else {
                    currentLine_.push_back(c);
                }

                if (target_) {
                    target_->sputc(c); // Best effort mirror; do not fail capture if output sink is unavailable.
                }
                return ch;
            }

            std::streamsize xsputn(const char* s, std::streamsize count) override {
                std::lock_guard<std::mutex> lock(lineMutex_);
                for (std::streamsize i = 0; i < count; ++i) {
                    const char c = s[i];
                    if (c == '\n') {
                        AppendBufferedLine(currentLine_);
                        currentLine_.clear();
                    }
                    else {
                        currentLine_.push_back(c);
                    }
                }
                if (target_) {
                    target_->sputn(s, count); // Best effort mirror.
                }
                return count;
            }

            int sync() override {
                std::lock_guard<std::mutex> lock(lineMutex_);
                return target_ ? target_->pubsync() : 0;
            }

        private:
            void FlushPendingLineUnlocked() {
                if (!currentLine_.empty()) {
                    AppendBufferedLine(currentLine_);
                    currentLine_.clear();
                }
            }
            std::streambuf* target_ = nullptr;
            std::string currentLine_;
            std::mutex lineMutex_;
        };

        std::unique_ptr<TeeCaptureBuf> gCoutCapture;
        std::unique_ptr<TeeCaptureBuf> gCerrCapture;
        std::unique_ptr<std::ostream> gCapturedCoutStream;
        std::unique_ptr<std::ostream> gCapturedCerrStream;

        void InitLogCapture() {
            gCoutCapture = std::make_unique<TeeCaptureBuf>(std::cout.rdbuf());
            std::cout.rdbuf(gCoutCapture.get());
            gCapturedCoutStream = std::make_unique<std::ostream>(gCoutCapture.get());

            gCerrCapture = std::make_unique<TeeCaptureBuf>(std::cerr.rdbuf());
            std::cerr.rdbuf(gCerrCapture.get());
            gCapturedCerrStream = std::make_unique<std::ostream>(gCerrCapture.get());
        }
    }

    const std::string Logger::LOGS_FILEEXT = "_crash.log";
    
    const std::string Logger::LOGS_FILEPATH = []() -> std::string {
        char path[MAX_PATH];
        // CSIDL_PERSONAL requests the "Documents" folder
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, path))) {
            // Create a subfolder
            std::filesystem::path fullPath = std::filesystem::path(path) / "Owala_Logs";

            // Create directory if it doesn't exist
            std::error_code ec;
            std::filesystem::create_directories(fullPath, ec);

            return fullPath.string() + "\\";
        }
        return "";
    }();

    /*****************************************************************//**
     * \brief   Initializes Windows debug symbols for accurate stack tracing.
     *********************************************************************/
    const void Logger::InitSymbols() {
        // SYMOPT_DEFERRED_LOADS: load symbols only when its needed
        // SYMOPT_LOAD_LINES: load file and line number information in addition to function names
        SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);

        if (!SymInitialize(GetCurrentProcess(),
            "srv*C:\\Symbols*https://msdl.microsoft.com/download/symbols",
            TRUE)) {
            Get().Error(DebugType::Logger) << "Failed to initialize symbols. Error: " << GetLastError() << std::endl;
        }
    }

    /*****************************************************************//**
     * \brief   Forms a timestamped filepath string for generating crash logs.
     *
     * \return  A string literal holding the generated filepath.
     *********************************************************************/
    const std::string Logger::GetCrashLogFilename() {
        std::ostringstream oss;
        std::time_t t = std::time(nullptr);
        std::tm tm = *std::localtime(&t);

        oss << LOGS_FILEPATH
            << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S")
            << LOGS_FILEEXT;
        return oss.str();
    }
    
    /*****************************************************************//**
     * \brief   Logs crash data utilizing a previously captured stack trace instance.
     *          Usually called when catching a specific custom GameException.
     *
     * \param   type
     * \param   message
     * \param   stack_trace
     *********************************************************************/
    // called when GameException is thrown. uses stack trace captured at the point where GameException was thrown
    void Logger::LogCrash(const DebugType type, const std::string& message, const backward::StackTrace& stack_trace) {
        Get().Error(type) << message << std::endl; // log to console for debugging

        if (Logger::LOGS_FILEPATH.empty()) {
            Get().Error(DebugType::Logger) << "Unable to get Documents directory. LOGS_FILEPATH: " << Logger::LOGS_FILEPATH << std::endl;
            return;
        }

        std::string logPath = GetCrashLogFilename();
        std::ofstream logFile(logPath, std::ios::app);

        if (!logFile) {
            Get().Error(DebugType::Logger) << "Unable to open " << logPath << std::endl;
        } else {
            backward::Printer p;
            p.object = true;
            p.address = true;
            p.print(stack_trace, stderr); // log to console for debugging

            // write to log file
            Get().Info(DebugType::Logger) << "Writing to " << logPath << std::endl;
            logFile << GetDebugType(type) << message << '\n' << std::endl;
            p.print(stack_trace, logFile);
        }
    }

    /*****************************************************************//**
     * \brief   Logs a generic crash dynamically, capturing a fresh stack
     *          trace exactly from the point of failure.
     *
     * \param   type
     * \param   message
     *********************************************************************/
    // called when other exceptions are thrown. captures stack trace at this point
    void Logger::LogCrash(const DebugType type, const std::string& message) {
        Get().Error(type) << message << std::endl; // log to console for debugging

        if (Logger::LOGS_FILEPATH.empty()) {
            Get().Error(DebugType::Logger) << "Unable to get Documents directory. LOGS_FILEPATH: " << Logger::LOGS_FILEPATH << std::endl;
            return;
        }

        std::string logPath = GetCrashLogFilename();
        std::ofstream logFile(logPath, std::ios::app);

        if (!logFile) {
            Get().Error(DebugType::Logger) << "Unable to open " << logPath << std::endl;
        }
        else {
            backward::StackTrace st; st.load_here(32);
            backward::Printer p;
            p.object = true;
            p.address = true;
            p.print(st, stderr); // log to console for debugging

            // write to log file
            Get().Info(DebugType::Logger) << "Writing to " << logPath << std::endl;
            logFile << GetDebugType(type) << message << '\n' << std::endl;
            p.print(st, logFile);
        }
    }

    /*****************************************************************//**
     * \brief   Outputs standard operational or status information to the console.
     *
     * \param   type
     * \return  A mutable stream reference ready to be pipelined.
     *********************************************************************/
    // Informative logging
    std::ostream& Logger::Info(DebugType type) {
        std::call_once(gLogCaptureInitOnce, InitLogCapture);
        if (gCapturedCoutStream) {
            gCapturedCoutStream->clear();
        }
        (*gCapturedCoutStream) << GetDebugType(type) << ' ';
        return *gCapturedCoutStream;
    }

    /*****************************************************************//**
     * \brief   Outputs structured error or failing execution logs to the console.
     *
     * \param   type
     * \return  A mutable stream reference pointing to the system error channel.
     *********************************************************************/
    // Error logging
    std::ostream& Logger::Error(DebugType type) {
        std::call_once(gLogCaptureInitOnce, InitLogCapture);
        if (gCapturedCerrStream) {
            gCapturedCerrStream->clear();
        }
        (*gCapturedCerrStream) << "[ERROR] " << GetDebugType(type) << ' ';
        return *gCapturedCerrStream;
    }

    std::vector<std::string> Logger::GetRecentLogs() {
        std::call_once(gLogCaptureInitOnce, InitLogCapture);
        if (gCoutCapture) gCoutCapture->FlushPendingLine();
        if (gCerrCapture) gCerrCapture->FlushPendingLine();
        std::lock_guard<std::mutex> lock(gLogBufferMutex);
        return { gBufferedLogLines.begin(), gBufferedLogLines.end() };
    }

    void Logger::ClearRecentLogs() {
        std::call_once(gLogCaptureInitOnce, InitLogCapture);
        std::lock_guard<std::mutex> lock(gLogBufferMutex);
        gBufferedLogLines.clear();
    }
}
