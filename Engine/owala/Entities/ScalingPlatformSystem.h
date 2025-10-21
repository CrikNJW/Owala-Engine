/*****************************************************************//**
* \file   ScalingPlatformSystem.h
*
* \primary_author
*      Sitara Anil
* \secondary_author
*
*
* \brief    System that manages the update cycle for all scaling platforms,
*           handling their animation and transformation updates each frame.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "ISystem.h"
#include "EntityManager.h"

namespace Framework {
    class ScalingPlatformSystem : public ISystem {
    private:
        EntityManager* ecs_;

    public:
        ScalingPlatformSystem(EntityManager& entityMgr) : ecs_(&entityMgr) {}

        void Initialize() override {}
        void Update(float dt) override;
        void Shutdown() override {}
    };
}