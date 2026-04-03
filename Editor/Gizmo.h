/*****************************************************************//**
* \file   Gizmo.h
*
* \primary_author
*       Yar Xuan (80%)
* \secondary_author
*      Jason Lou (20%)
*
* \brief    Header file for Gizmo system in the Editor module.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include <imgui.h>
#include <string>
#include <vector>
#include "Entity.h"

// forward declarations
namespace Framework {
    enum ComponentType : int;
    class EntityStorage;
    class CameraSystem;
}

namespace Editor {
    class Editor; // forward declaration

    /*****************************************************************//**
     * \enum    GizmoMode
     * \brief   Represents the current active transformation mode of the gizmo.
     *********************************************************************/
    enum class GizmoMode { None, Translate, Rotate, Scale };

    /*****************************************************************//**
     * \enum    GizmoAxis
     * \brief   Represents the specific axis or interaction handle of a gizmo.
     *********************************************************************/
    enum class GizmoAxis { None, X, Y, Center, RotateRing };

    /*****************************************************************//**
     * \class   GizmoSystem
     * \brief   System responsible for rendering and managing interactions with 
     *          on-screen editor manipulation tools (translate, scale, rotate) 
     *          and invisible entity selection placeholders.
     *********************************************************************/
    class GizmoSystem {
    public:
        // The current transformation active mode.
        GizmoMode currentMode_ = GizmoMode::None;

        /*****************************************************************//**
         * \brief   Constructs a GizmoSystem attached to the main Editor.
         *
         * \param   editor Reference to the main level editor containing state.
         *********************************************************************/
        explicit GizmoSystem(Editor& editor);

        // --- Invisible Entities stuff ---
        
        /*****************************************************************//**
         * \brief   Draws a small green dot placeholder for invisible entities 
         *          (like Spawn Points or Invisible Colliders) to make them 
         *          selectable in the viewport.
         *
         * \param   drawList ImGui draw list used to render the shapes.
         *********************************************************************/
        void DrawInvisPlaceholders(ImDrawList* drawList);

        /*****************************************************************//**
         * \brief   Checks whether the mouse click currently falls over an 
         *          invisible entity placeholder.
         *
         * \return  The Entity ID if an invisible object is clicked; 0 otherwise.
         *********************************************************************/
        Entity IsInvisEntityClicked();


        // --- Gizmos Drawing ---

        /*****************************************************************//**
         * \brief   Main render loop function to draw the active transform 
         *          gizmo over the currently selected entity.
         *
         * \param   drawList Pointer to the active ImGui draw list.
         * \param   selectedEntityIndex The array index mapping to the chosen entity.
         *********************************************************************/
        void DrawGizmos(ImDrawList* drawList, int selectedEntityIndex);

        /*****************************************************************//**
         * \brief   Draws a selection visual outline bound around an entity.
         *
         * \param   drawList Pointer to the ImGui draw list.
         * \param   entity Target entity to outline.
         *********************************************************************/
        void DrawOutline(ImDrawList* drawList, Entity entity);

        /*****************************************************************//**
         * \brief   Draws the interactive translation arrows (X & Y).
         *
         * \param   drawList Pointer to the ImGui draw list.
         * \param   origin Screen-space positioning base coordinate.
         *********************************************************************/
        void DrawTranslateGizmo(ImDrawList* drawList, ImVec2 origin);

        /*****************************************************************//**
         * \brief   Draws the interactive rotation circle ring.
         *
         * \param   drawList Pointer to the ImGui draw list.
         * \param   origin Screen-space positioning base coordinate.
         * \param   entity The entity being rotated (for current values).
         *********************************************************************/
        void DrawRotateGizmo(ImDrawList* drawList, ImVec2 origin, Entity entity);

        /*****************************************************************//**
         * \brief   Draws the interactive scaling handles (boxes).
         *
         * \param   drawList Pointer to the ImGui draw list.
         * \param   origin Screen-space positioning base coordinate.
         *********************************************************************/
        void DrawScaleGizmo(ImDrawList* drawList, ImVec2 origin);


        // --- Gizmos Dragging (Actual transformation logic) ---

        /*****************************************************************//**
         * \brief   Standard per-frame update handling gizmo drag state machine 
         *          (start, drag, stop operations).
         *
         * \param   selectedEntityIndex The target entity being modified.
         * \param   activeCamera The camera utilized for screen/world conversion.
         *********************************************************************/
        void Update(int selectedEntityIndex, Entity activeCamera);

        /*****************************************************************//**
         * \brief   Finalizes dragging operations, generating undo/redo states 
         *          for tracked modifications and committing modifications.
         *********************************************************************/
        void HandleStopDrag();

        /*****************************************************************//**
         * \brief   Registers snapshot parameters before modifications begin 
         *          to facilitate restoration if needed via history.
         *********************************************************************/
        void HandleStartDrag();

        /*****************************************************************//**
         * \brief   Injects dimensional, angular or translational shifts into 
         *          the components attached to the modified entity based on mouse 
         *          screen delta shifts.
         *
         * \param   mouseDelta Change ratio per update iteration in screen pixels.
         * \param   activeCamera The reference camera tracking the scale relative offsets.
         *********************************************************************/
        void ApplyTransformation(const ImVec2 mouseDelta, const Entity activeCamera);

        
        // --- Helper functions ---

        /*****************************************************************//**
         * \brief   Evaluates if the Gizmo tool is activated.
         *
         * \return  True if current tool mode is not None.
         *********************************************************************/
        bool IsGizmosToggled() const {
            return currentMode_ != GizmoMode::None;
        }
        
        /*****************************************************************//**
         * \brief   Evaluates if an interactive part of the gizmo is hovered.
         *
         * \return  True if an axis constraint is targeted by the cursor.
         *********************************************************************/
        bool IsGizmosHover() const { return hoveredAxis_ != GizmoAxis::None; }

        /*****************************************************************//**
         * \brief   Updates internal enumeration corresponding to operational mode.
         *
         * \param   mode Desired tool variant (Scale, Translate, Rotate).
         *********************************************************************/
        void SetGizmoMode(GizmoMode mode) {
            currentMode_ = mode;
        }

    private:
        Editor* editor_ = nullptr;

        // Circle radius for placeholder for invisible entities
        const float invisCircleRad_ = 8.0f;

        // Gizmo Dimensions
        const float axisLength_ = 80.0f; // The arrows are always 80px long
        const float axisThickness_ = 8.0f;
        const float handleSize_ = 10.0f; // Size of the square/triangle at tip
        const float centerCubeHalfsize_ = 8.0f;

        // Stores which axis is currently being hovered
        GizmoAxis hoveredAxis_ = GizmoAxis::None;
        // Stores which axis is currently active (being clicked on)
        GizmoAxis activeAxis_ = GizmoAxis::None; 
        // True when the mouse is down and we are actively modifying an entity
        bool isDragging_ = false;
        // Stores the mouse position from the *previous* frame for delta calculation
        ImVec2 lastMousePos_; 
        // Stores the selected entity ID for easy access during the drag
        Entity draggedEntity_ = 0;

        // For Undo/Redo system (HistoryManager)
        std::vector<Framework::ComponentType> gizmoAffectedComponents_;
        std::vector<std::string> beforeGizmosJSON_; 
        std::vector<std::string> afterGizmosJSON_;
    };

}