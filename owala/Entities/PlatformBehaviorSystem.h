/*******************************************************************
* \file   PlatformBehaviorSystem.h
*
* \primary_author
*      Sitara Anil
* \secondary_author
*
*
* \brief
*      System for managing special platform behaviors including trap
*      platforms that fall after a trigger delay
* 
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "Entity.h"
#include "Matrix3x3.h"

namespace Framework {
    class TransformationSystem;

    /*****************************************************************//**
     * \class   PlatformBehaviorSystem
     * \brief   System responsible for updating and managing specialized 
     *          platform logic, such as trap platforms that react to collision 
     *          and fall when triggered.
     *********************************************************************/
    class PlatformBehaviorSystem : public ISystem {
    private:
        /*****************************************************************//**
         * \brief   Evaluates whether a specific trap platform has been 
         *          triggered and should begin falling.
         *
         * \return  True if the platform's trigger conditions are met.
         *********************************************************************/
        bool ShouldTrapFall(EntityManager& entityMgr, Entity entity, float worldRotation) const;

        /*****************************************************************//**
         * \brief   Iterates through and processes the state machines of all 
         *          active trap platforms (e.g., shaking, falling).
         *********************************************************************/
        void UpdateTrapPlatforms();

    public:
        PlatformBehaviorSystem() = default;

        void Initialize() override {}

        /*****************************************************************//**
         * \brief   Main update loop for the system, processing platform 
         *          behaviors each frame.
         *********************************************************************/
        void Update() override;

        void Shutdown() override {}

        // Trap platform specific methods

        /*****************************************************************//**
         * \brief   Resets a triggered or fallen trap platform back to its 
         *          original un-triggered anchor state and position.
         *********************************************************************/
        void ResetTrap(Entity trapPlatform);

    };
}