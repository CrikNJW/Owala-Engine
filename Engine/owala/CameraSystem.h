#pragma once
#include "ISystem.h"
#include "Components.h"  // ? THIS IS CRITICAL - includes Camera struct
#include <memory>
#include "entity.h"

namespace Framework {
    class EntityStorage;
}

namespace Framework {

    class CameraSystem : public ISystem {
    public:
        CameraSystem(EntityStorage& ecs);

        void Initialize() override {}
        void Update(float dt) override;
        void Shutdown() override {}

        Vector2D WorldToScreen(const Vector2D& worldPos, Entity cameraEntity = 0);
        Vector2D ScreenToWorld(const Vector2D& screenPos, Entity cameraEntity = 0);

        // Camera access methods
        Entity GetActiveCamera() const;
        Entity GetGameCamera() const;
        Entity GetEditorCamera() const;

        void SetActiveCamera(Entity cameraEntity);
        void GetViewBounds(Entity cameraEntity, float& left, float& right, float& bottom, float& top);

        // Camera creation with domain support
        Entity CreateCamera(const Vector2D& position = { 0.0f, 0.0f },
            Entity targetEntity = 0,
            bool isActive = false,
            Camera::Domain domain = Camera::Domain::GAME);

        // Camera configuration
        void SetZoom(Entity cameraEntity, float zoom);
        void SetFollowTarget(Entity cameraEntity, Entity targetEntity, float followSpeed = 5.0f);
        void SetCameraBounds(Entity cameraEntity, const Vector2D& min, const Vector2D& max);

        // Editor camera controls
        void PanCamera(Entity camera, const Vector2D& delta);
        void ZoomCamera(Entity camera, float zoomDelta, const Vector2D& focusPoint = { 0,0 });
        void FocusOnPoint(Entity camera, const Vector2D& worldPos);

    private:
        Camera& GetCamera(Entity cameraEntity);

        EntityStorage& ecs_;
        Entity activeCamera_ = 0;
        Entity gameCamera_ = 0;    // Track the main game camera
        Entity editorCamera_ = 0;  // Track the editor camera
    };

} // namespace Framework