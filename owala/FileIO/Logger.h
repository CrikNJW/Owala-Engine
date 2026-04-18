/*****************************************************************//**
 * \file   Logger.h
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
#pragma once
#include <iostream>
#include <fstream>
#include <sstream>  
#include <ctime>
#include <vector>
#include "backward.hpp"
#include <windows.h>

/*****************************************************************//**
 * \enum    DebugType
 * \brief   Enumeration categorizing the source subsystem or context
 *          of a log message or generated exception.
 *********************************************************************/
const enum class DebugType {
    Unknown,
    WindowsSystem,
    InputSystem,
    PhysicsSystem,
    AudioSystem,
    SceneManager,
    EntityManager,
    TransformationSystem,
    PlayerSystem,
    GraphicsSystem,
    SpriteRenderSystem,
    TextRenderSystem,
    ResourceManager,
    GameManager,
    FileIOSystem,
    Logger,
    Editor,
    Font,
    EntitySeralizer,
    AISystem,
    GUISystem,
    VFXSystem,
	CameraSystem,
	FireBaseManager,
    Game
};

/*****************************************************************//**
 * \var     DebugTypeLabels
 * \brief   String representations corresponding sequentially to
 *          the entries defined in the DebugType enumeration.
 *********************************************************************/
const std::string DebugTypeLabels[] = {
    "Unknown Exception",
    "WindowsSystem",
    "InputSystem",
    "PhysicsSystem",
	"AudioSystem",
    "SceneManager",
    "EntityManager",
    "TransformationSystem",
    "PlayerSystem",
    "GraphicsSystem",
    "SpriteRenderSystem",
    "TextRenderSystem",
    "ResourceManager",
    "GameManager",
    "FileIOSystem",
    "Logger",
    "Editor",
    "Font",
    "EntitySeralizer",
    "AISystem",
    "GUISystem",
    "VFXSystem",
	"CameraSystem",
    "FireBaseManager",
    "Game"
};

namespace Framework {

    // throw GameException with DebugType and a msg instead of runtime_error
    
    /*****************************************************************//**
     * \struct  GameException
     * \brief   A specialized exception block capturing deep tracing paths 
     *          automatically exactly when the throw executes.
     *********************************************************************/
    const struct GameException : public std::exception {
        DebugType type;
        std::string msg;
        backward::StackTrace stack_trace;

        GameException(DebugType c, std::string m) : type(c), msg(std::move(m)) {
            stack_trace.load_here(); // capture the stack at the point when it was thrown
        }

        const char* what() const noexcept override {
            return msg.c_str();
        }
    };

    /*****************************************************************//**
     * \class   Logger
     * \brief   Singleton class directing logging streams and establishing 
     *          crash serialization outputs formatting stack traces tracking execution.
     *********************************************************************/
	class Logger {
    public:
        /*****************************************************************//**
         * \brief   need this to resolve the symbols in stack trace
         *********************************************************************/
        static const void InitSymbols();

        /*****************************************************************//**
         * \brief   Retrieves globally mapped core logger singleton instance.
         *
         * \return  Singleton Logger instance.
         *********************************************************************/
        static Logger& Get() {
            static Logger instance;
            return instance;
        }

        /*****************************************************************//**
         * \brief   Directs standard info debugging outputs sequentially dynamically formats structures.
         *
         * \param   type
         * \return  Stream logic outputs formatting text paths visually mapping console streams cleanly.
         *********************************************************************/
        static std::ostream& Info(DebugType type);
        
        /*****************************************************************//**
         * \brief   Captures error reporting text dynamically formatting debugging streams securely paths explicitly.
         *
         * \param   type
         *********************************************************************/
        static std::ostream& Error(DebugType type);

        // Returns a snapshot of recent console log lines.
        static std::vector<std::string> GetRecentLogs();
        // Clears buffered log lines used by the in-editor debug console.
        static void ClearRecentLogs();
        
        /*****************************************************************//**
         * \brief   Registers tracking configurations logically explicitly explicitly.
         *
         * \param   type
         * \param   message
         * \param   stack_trace
         *********************************************************************/
        static void LogCrash(const DebugType type, const std::string& message, const backward::StackTrace& stack_trace);
        
        /*****************************************************************//**
         * \brief   Records explicitly defined trace failures optimally precisely uniquely gracefully smoothly organically manually.
         *
         * \param   type
         *********************************************************************/
        static void LogCrash(const DebugType type, const std::string& message);

    private:
        Logger() = default;

        static const std::string LOGS_FILEPATH, LOGS_FILEEXT;

        static const std::string GetDebugType(DebugType type) {
            return '[' + DebugTypeLabels[(int)type] + "] ";
        }

        static const std::string GetCrashLogFilename();
	};
} // namespace Framework
