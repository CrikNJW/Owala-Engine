/*****************************************************************//**
 * \file   SpotlightSystem.cpp
 *
 * \primary_author
 *       Jordain Ng Ying Zhe (100%)
 *
 * \brief  Two-pass spotlight rendering:
 *         Pass 1 — render all textured entities as white rectangles
 *                  into an offscreen occluder FBO (shadow mask).
 *         Pass 2 — composite a spotlight cone over the scene using
 *                  the occluder mask for shadow ray-marching.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "pch.h"
#include "SpotlightSystem.h"
#include "GameManager.h"
#include "EntityManager.h"
#include "Components.h"
#include "Graphics.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Framework {

    void SpotlightSystem::Update() {
        GameManager& gm = GameManager::GetInstance();
        Graphics& gfx = *gm.GetSystems().gfx_;
        EntityManager entityMgr(gm.GetECS());
        EntityStorage& storage = gm.GetECS();

        // Collect all spotlight entities
        std::vector<Entity> spotlightEntities =
            entityMgr.GetAllEntitiesWithComponent(true, SpotlightComponentType);

        if (spotlightEntities.empty()) return;

        for (Entity entity : spotlightEntities) {
            SpotlightComponent& spotlight = entityMgr.GetComponent<SpotlightComponent>(entity);
            if (!spotlight.isActive) continue;

            // --- Get light world position ---
            if (!entityMgr.HasComponent(entity, TransformType)) continue;
            const Vector2D& worldPos = entityMgr.GetComponent<Transform>(entity).position;

            // --- Auto-flip direction from PlayerState ---
            float dirRad = spotlight.directionAngle * static_cast<float>(M_PI / 180.0);
            if (spotlight.followEntityFacing && entityMgr.HasComponent(entity, PlayerStateType)) {
                const PlayerState& ps = entityMgr.GetComponent<PlayerState>(entity);
                dirRad = ps.isFacingRight ? 0.0f : static_cast<float>(M_PI);
            }

            // --- Convert light position + radius to NDC ---
            Vector2D lightNDC;
            WorldToNDC(worldPos, lightNDC);
            float radiusNDC = WorldRadiusToNDC(spotlight.radius);

            // =======================================================
            // PASS 1 — Render occluders into the shadow FBO
            // =======================================================
            gfx.BindOccluderFBO();

            // Iterate every entity that has a texture — they are shadow casters
            std::vector<Entity> texturedEntities =
                entityMgr.GetAllEntitiesWithComponent(true, TextureComponentType);

            for (Entity occEntity : texturedEntities) {
                if (!entityMgr.HasComponent(occEntity, TransformType)) continue;

                const Transform& occTrans = entityMgr.GetComponent<Transform>(occEntity);
                const TextureComponent& occTex = entityMgr.GetComponent<TextureComponent>(occEntity);

                Vector2D occNDC;
                WorldToNDC(occTrans.position, occNDC);

                // Convert half-sizes to NDC
                float hw = WorldRadiusToNDC(occTex.HalfSize.x * occTrans.scale.x);
                float hh = WorldRadiusToNDC(occTex.HalfSize.y * occTrans.scale.y);

                gfx.RenderOccluderRect(occNDC.x, occNDC.y, hw, hh);
            }

            gfx.UnbindOccluderFBO();

            // =======================================================
            // PASS 2 — Composite spotlight over the scene
            // =======================================================
            float coneRad = spotlight.coneAngle * static_cast<float>(M_PI / 180.0);

            gfx.CompositeSpotlight(
                lightNDC,
                radiusNDC,
                coneRad,
                dirRad,
                spotlight.color,
                spotlight.intensity,
                spotlight.falloff,
                spotlight.ambientColor,
                spotlight.shadowRays,
                spotlight.castShadows
            );
        }
    }

    // -----------------------------------------------------------------------
    // WorldToNDC
    // Converts a world-space Vector2D into NDC [-1, 1] using the active camera.
    // -----------------------------------------------------------------------
    void SpotlightSystem::WorldToNDC(const Vector2D& worldPos, Vector2D& outNDC) const {
        CameraSystem* camSys = GameManager::GetInstance().GetSystems().camera_;
        if (!camSys) { outNDC = { 0.f, 0.f }; return; }

        Entity activeCam = camSys->GetActiveCamera();
        if (activeCam == 0) { outNDC = { 0.f, 0.f }; return; }

        const Camera& cam = camSys->GetCamera(activeCam);
        float halfW = (cam.size.x * 0.5f) / cam.zoom;
        float halfH = (cam.size.y * 0.5f) / cam.zoom;

        outNDC.x = (worldPos.x - cam.position.x) / halfW;
        outNDC.y = (worldPos.y - cam.position.y) / halfH;
    }

    // -----------------------------------------------------------------------
    // WorldRadiusToNDC
    // Converts a world-space scalar distance to an NDC scalar using the
    // active camera's half-width as the reference unit.
    // -----------------------------------------------------------------------
    float SpotlightSystem::WorldRadiusToNDC(float worldRadius) const {
        CameraSystem* camSys = GameManager::GetInstance().GetSystems().camera_;
        if (!camSys) return worldRadius;

        Entity activeCam = camSys->GetActiveCamera();
        if (activeCam == 0) return worldRadius;

        const Camera& cam = camSys->GetCamera(activeCam);
        float halfW = (cam.size.x * 0.5f) / cam.zoom;
        return worldRadius / halfW;
    }

} // namespace Framework