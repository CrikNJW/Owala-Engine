#include "CameraSystem.h"
#include "EntityStorage.h"
#include "EntityManager.h"
#include <cmath>
#include <stdexcept>

namespace Framework {

    CameraSystem::CameraSystem(EntityStorage& ecs) : ecs_(ecs) {
        // Constructor
    }

    void CameraSystem::Update(float dt) {
        EntityManager entityMgr(ecs_);

        for (const Entity& e : entityMgr.GetAllEntitiesWithComponent(CameraType)) {
            Camera& camera = entityMgr.GetComponent<Camera>(e);
            camera.size = { 2.0f * camera.zoom, 2.0f * camera.zoom };

            // Only update game cameras with follow behavior
            if (camera.domain == Camera::Domain::GAME &&
                camera.targetEntity != 0 &&
                entityMgr.HasComponent(camera.targetEntity, PositionType)) {

                const auto& targetPos = entityMgr.GetComponent<Position>(camera.targetEntity).Pos;

                if (camera.followSpeed <= 0.0f) {
                    camera.position = targetPos;
                }
                else {
                    float t = 1.0f - std::exp(-camera.followSpeed * dt);
                    camera.position.x += (targetPos.x - camera.position.x) * t;
                    camera.position.y += (targetPos.y - camera.position.y) * t;

                    // Apply bounds for game camera
                    camera.position.x = std::max(camera.boundsMin.x, std::min(camera.position.x, camera.boundsMax.x));
                    camera.position.y = std::max(camera.boundsMin.y, std::min(camera.position.y, camera.boundsMax.y));
                }
            }
            // Editor cameras don't auto-update - they're manually controlled
        }
    }

    Vector2D CameraSystem::WorldToScreen(const Vector2D& worldPos, Entity cameraEntity) {
        Camera& camera = GetCamera(cameraEntity);
        Vector2D screenPos;

        // Only transform position, don't affect size
        screenPos.x = (worldPos.x - camera.position.x) / (camera.size.x * 0.5f * camera.zoom);
        screenPos.y = (worldPos.y - camera.position.y) / (camera.size.y * 0.5f * camera.zoom);

        return screenPos;
    }

    Vector2D CameraSystem::ScreenToWorld(const Vector2D& screenPos, Entity cameraEntity) {
        Camera& camera = GetCamera(cameraEntity);
        Vector2D worldPos;
        worldPos.x = camera.position.x + (screenPos.x * camera.size.x * 0.5f);
        worldPos.y = camera.position.y + (screenPos.y * camera.size.y * 0.5f);
        return worldPos;
    }

    Entity CameraSystem::GetActiveCamera() const {
        return activeCamera_;
    }

    Entity CameraSystem::GetGameCamera() const {
        return gameCamera_;
    }

    Entity CameraSystem::GetEditorCamera() const {
        return editorCamera_;
    }

    void CameraSystem::SetActiveCamera(Entity cameraEntity) {
        EntityManager entityMgr(ecs_);

        if (activeCamera_ != 0 && entityMgr.HasComponent(activeCamera_, CameraType)) {
            entityMgr.GetComponent<Camera>(activeCamera_).isActive = false;
        }

        if (cameraEntity != 0 && entityMgr.HasComponent(cameraEntity, CameraType)) {
            entityMgr.GetComponent<Camera>(cameraEntity).isActive = true;
            activeCamera_ = cameraEntity;
        }
        else {
            activeCamera_ = 0;
        }
    }

    void CameraSystem::GetViewBounds(Entity camera, float& left, float& right, float& bottom, float& top) {
        EntityManager entityMgr(ecs_);

        if (entityMgr.HasComponent(camera, CameraType)) {
            Camera& cam = entityMgr.GetComponent<Camera>(camera);

            // Account for zoom in view bounds calculation
            float zoomedWidth = cam.size.x * cam.zoom;
            float zoomedHeight = cam.size.y * cam.zoom;

            left = cam.position.x - zoomedWidth * 0.5f;
            right = cam.position.x + zoomedWidth * 0.5f;
            bottom = cam.position.y - zoomedHeight * 0.5f;
            top = cam.position.y + zoomedHeight * 0.5f;
        }
        else {
            // Default bounds if camera not found
            left = -1.0f; right = 1.0f; bottom = -1.0f; top = 1.0f;
        }
    }

    Entity CameraSystem::CreateCamera(const Vector2D& position, Entity targetEntity, bool isActive, Camera::Domain domain) {
        EntityManager entityMgr(ecs_);
        Entity cameraEntity = entityMgr.CreateEntity();

        Camera camera;
        camera.position = position;
        camera.targetEntity = targetEntity;
        camera.isActive = isActive;
        camera.domain = domain;

        // Set domain-specific defaults
        if (domain == Camera::Domain::EDITOR) {
            camera.moveSpeed = 10.0f;
            camera.zoomSpeed = 2.0f;
        }

        entityMgr.AddComponent(cameraEntity, camera);

        // Track domain-specific cameras
        if (domain == Camera::Domain::GAME) {
            gameCamera_ = cameraEntity;
        }
        else {
            editorCamera_ = cameraEntity;
        }

        if (isActive) {
            SetActiveCamera(cameraEntity);
        }

        return cameraEntity;
    }

    void CameraSystem::SetZoom(Entity cameraEntity, float zoom) {
        Camera& camera = GetCamera(cameraEntity);
        camera.zoom = std::max(0.1f, zoom);
    }

    void CameraSystem::SetFollowTarget(Entity cameraEntity, Entity targetEntity, float followSpeed) {
        Camera& camera = GetCamera(cameraEntity);
        camera.targetEntity = targetEntity;
        camera.followSpeed = followSpeed;
    }

    void CameraSystem::SetCameraBounds(Entity cameraEntity, const Vector2D& min, const Vector2D& max) {
        Camera& camera = GetCamera(cameraEntity);
        camera.boundsMin = min;
        camera.boundsMax = max;
    }

    // Editor-specific controls
    void CameraSystem::PanCamera(Entity camera, const Vector2D& delta) {
        Camera& cam = GetCamera(camera);
        cam.position.x += delta.x * cam.zoom;
        cam.position.y += delta.y * cam.zoom;
    }

    void CameraSystem::ZoomCamera(Entity camera, float zoomDelta, const Vector2D& focusPoint) {
        Camera& cam = GetCamera(camera);
        float oldZoom = cam.zoom;
        cam.zoom = std::max(0.1f, cam.zoom + zoomDelta);

        // Zoom towards focus point (usually mouse position)
        if (focusPoint.x != 0 || focusPoint.y != 0) {
            float zoomFactor = cam.zoom / oldZoom;
            cam.position.x = focusPoint.x - (focusPoint.x - cam.position.x) * zoomFactor;
            cam.position.y = focusPoint.y - (focusPoint.y - cam.position.y) * zoomFactor;
        }
    }

    void CameraSystem::FocusOnPoint(Entity camera, const Vector2D& worldPos) {
        Camera& cam = GetCamera(camera);
        cam.position = worldPos;
    }

    Camera& CameraSystem::GetCamera(Entity cameraEntity) {
        EntityManager entityMgr(ecs_);

        if (cameraEntity == 0) {
            cameraEntity = activeCamera_;
        }

        if (cameraEntity == 0 || !entityMgr.HasComponent(cameraEntity, CameraType)) {
            throw std::runtime_error("CameraSystem: Camera not found");
        }

        return entityMgr.GetComponent<Camera>(cameraEntity);
    }

} // namespace Framework