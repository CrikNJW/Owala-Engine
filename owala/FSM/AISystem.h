/*********************************************************************//**
* \file   AISystem.h
*
* \primary_author
* Lur Pei Hao Stanley
* \secondary_author
*
*
* \brief
* Declares the AISystem class, which orchestrates the AI logic updates,
* manages the navigation grid, and integrates AI components with the ECS.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "Pathfinding.h"
#include "Components.h"
#include "FSMFactory.h" 

#ifdef BUILD_EDITOR
#include "Editor.h"
#include "imgui.h"
#endif

namespace Framework {

    /*****************************************************************//**
     * \class   AISystem
     * \brief   System that handles AI logic, state machines, and navigation.
     *********************************************************************/
    class AISystem : public ISystem {
    public:
        AISystem() = default;

        /*****************************************************************//**
         * \brief   Sets up the AI system.
         *********************************************************************/
        void Initialize() override;

        /*****************************************************************//**
         * \brief   Updates AI entity logic each frame.
         *********************************************************************/
        void Update() override;

        /*****************************************************************//**
         * \brief   Cleans up the AI system before shutting down.
         *********************************************************************/
        void Shutdown() override;

        /*****************************************************************//**
         * \brief   Manually forces the navigation grid to rebuild itself.
         *********************************************************************/
        inline void ForceGridRebuild() { navGrid_.BuildGrid(); }

#ifdef BUILD_EDITOR
        /*****************************************************************//**
         * \brief   Draws AI debug visuals in the level editor.
         * 
         * \param   cameraSystem 
         * \param   drawList 
         * \param   editor 
         * \param   cameraToUse 
         *********************************************************************/
        void DrawDebug(CameraSystem* cameraSystem, ImDrawList* drawList, Editor::Editor* editor, Entity cameraToUse);
        
        bool debugVisualization = false;
        bool showNavigationGrid = false;
        bool showFSMStateDebug = false;
        bool showCombatDebug = false;
#endif

    private:
        NavigationGrid navGrid_;
        // The factory must be a member so it can share the NavGrid with states
        std::unique_ptr<FSMFactory> fsmFactory_;

        float gridRebuildTimer_ = 0.0f;
        const float gridRebuildInterval_ = 2.0f;

        // Helper to initialize FSMs for new entities
        /*****************************************************************//**
         * \brief   Sets up the finite state machine for a specific entity.
         * 
         * \param   entity 
         * \param   fsmComp 
         * \param   ecs 
         *********************************************************************/
        void InitializeEntityFSM(Entity entity, FSMComponent& fsmComp, EntityStorage& ecs);

        // Debug helpers...
#ifdef BUILD_EDITOR
        //void DrawNavigationGrid(float viewLeft, float viewRight, float viewBottom, float viewTop, ImDrawList* drawList, Editor::Editor* editor, Entity cameraID);
        // Updated to use FSMComponent
        
        /*****************************************************************//**
         * \brief   Draws specific debug information for basic FSMs.
         * 
         * \param   entity 
         * \param   fsmComp 
         * \param   pos 
         * \param   drawList 
         * \param   editor 
         * \param   cameraID 
         *********************************************************************/
        void DrawFSMDebug(Entity entity, const FSMComponent& fsmComp, const Vector2D& pos, ImDrawList* drawList, Editor::Editor* editor, Entity cameraID);
        
        /*****************************************************************//**
         * \brief   Draws combat-oriented debug information for FSM entities.
         * 
         * \param   entity 
         * \param   fsmComp 
         * \param   pos 
         * \param   drawList 
         * \param   editor 
         * \param   cameraID 
         *********************************************************************/
        void DrawCombatDebug(Entity entity, const FSMComponent& fsmComp, const Vector2D& pos, ImDrawList* drawList, Editor::Editor* editor, Entity cameraID);
#endif
    };
}
