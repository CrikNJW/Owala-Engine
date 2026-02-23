/*****************************************************************//**
 * \file   SpotlightSystem.h
 *
 * \primary_author
 *       Jordain Ng Ying Zhe (100%)
 *
 * \brief  Renders spotlight effects with shadow casting using a
 *         two-pass occluder + composite approach.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once
#include "ISystem.h"
#include "Entity.h"
#include "Vector2D.h"

namespace Framework {

    class SpotlightSystem : public ISystem {
    public:
        void Update() override;

    private:
        // Convert a world-space position to NDC [-1, 1] using the active camera
        void WorldToNDC(const Vector2D& worldPos, Vector2D& outNDC) const;

        // Convert a world-space radius to an NDC radius using the active camera's half-width
        float WorldRadiusToNDC(float worldRadius) const;
    };

} // namespace Framework