/*****************************************************************//**
* \file   TrapPlatform.h
*
* \primary_author
*      Sitara Anil
* \secondary_author
*
*
* \brief Provides functionality for trap platforms that fall after a delay
*      when triggered. Includes creation, updating, triggering, and
*      resetting of trap platform entities.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/

#pragma once
#include "Entity.h"
#include "Vector2D.h"
#include <string>
#include <Components.h>

namespace Framework {
    struct TrapPlatformComponent;
    class EntityManager;
    void updateTrapPlatforms(EntityManager& entityMgr, float dt);
    void triggerTrapPlatform(EntityManager& entityMgr, Entity trapPlatform);
    void resetTrapPlatform(EntityManager& entityMgr, Entity trapPlatform);
}