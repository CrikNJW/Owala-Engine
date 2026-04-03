/*****************************************************************//**
 * \file   RopeScript.h
 *
 * \primary_author Sitara Anil
 * 
 * \secondary_author
 *
 *
 * \brief  NativeScript implementation for rope attachment to platforms.
 *         Vertical platforms: 2 ropes (left and right sides)
 *         Horizontal platforms: 1 rope (top center)
 *         MultiAxis platforms: NO ropes
 *
 *         Ropes are fixed at the nearest tiles above/beside the
 *         platform's original position and stretch/compress as the
 *         platform moves.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once

#include "ScriptableEntity.h"
#include "Components.h"
#include "EntityManager.h"
#include <vector>
#include <string>
#include <unordered_map>

using namespace Framework;

/*****************************************************************//**
 * \class   RopeScript
 * \brief   Script that manages connecting ropes for moving platforms.
 *********************************************************************/
class RopeScript : public ScriptableEntity {
public:
    static constexpr float MIN_ROPE_LENGTH = 20.0f;
    static constexpr float ROPE_OVERHEAD = 20.0f;
    static constexpr float HORIZONTAL_FALLBACK_LENGTH = 160.0f;

protected:
    /*****************************************************************//**
     * \brief   Sets up initial values and geometry logic.
     *********************************************************************/
    void OnCreate() override;

    /*****************************************************************//**
     * \brief   Updates connection lengths and positions each frame.
     *********************************************************************/
    void OnUpdate() override;

    /*****************************************************************//**
     * \brief   Free allocations and destroys rope entities.
     *********************************************************************/
    void OnDestroy() override;

private:
    /*****************************************************************//**
     * \struct  RopeRuntimeData
     * \brief   Tracks physical and visual state for an individual rope.
     *********************************************************************/
    struct RopeRuntimeData {
        Entity attachedPlatform = 0;
        Entity renderEntity = 0;
        std::string textureKey = "SB_PlatformRope.png";
        float originalLength = 0.0f;
        float currentLength = 0.0f;
        Vector2D ceilingAttachmentPoint = { 0.0f, 0.0f };
        Vector2D renderCeilingAttachmentPoint = { 0.0f, 0.0f };
        Vector2D platformAttachmentPoint = { 0.0f, 0.0f };
        Entity ceilingAnchorEntity = 0;
        bool isActive = true;
        float ropeWidth = 200.0f;
        float selectionHalfWidth = 7.0f;
        float segmentHeight = 20.0f;
        int ropeIndex = 0;
        bool hasFoundCeiling = true;
        float lastWorldRotationDeg = 0.0f;
    };

    // Helper methods
    
    /*****************************************************************//**
     * \brief   Retrieves the level's entity manager safely.
     *********************************************************************/
    EntityManager* GetEntityManager() const;
    //RopeRuntimeData* FindRopeRuntime(Entity rope) const;
    //void DestroyTrackedRope(EntityManager& entityMgr, Entity rope, Entity renderRope) const;
    //void SyncRenderRope(EntityManager& entityMgr, const RopeRuntimeData& runtimeData,
    //    const Transform& proxyTransform, float angleDeg) const;

    /*****************************************************************//**
     * \brief   Confirms if this setup requires dual ropes.
	 *********************************************************************/
    bool HasTwoRopes(EntityManager& entityMgr) const;

    /*****************************************************************//**
     * \brief   Confirms this specific object should generate connecting elements.
     *********************************************************************/
    bool IsValidRopePlatform() const;

    /*****************************************************************//**
     * \brief   Confirms vertical movement routing for top anchoring.
     *********************************************************************/
    bool FallsVertically() const;

    /*****************************************************************//**
     * \brief   Confirms if rope offsets align strictly to upward ceiling tiles.
     *********************************************************************/
    bool UsesCeilingTopRope(EntityManager& entityMgr) const;

    /*****************************************************************//**
     * \brief   Resolves dynamic offsets based on platform dimensions.
     *********************************************************************/
    Vector2D CalculateTopColliderAttachment(EntityManager& entityMgr, int ropeIndex) const;

    /*****************************************************************//**
     * \brief   Wipes old lines and calculates new ones.
     *********************************************************************/
    void InitializeRopesForPlatform();

    /*****************************************************************//**
     * \brief   Purges spawned data directly from ECS.
     *********************************************************************/
    void RemoveAllRopes();

    /*****************************************************************//**
     * \brief   Creates one indexed connecting line mapping variables sequentially.
     *********************************************************************/
    void CreateSingleRope(int ropeIndex);

    /*****************************************************************//**
     * \brief   Computes physical adjustments updating lengths and rendering logic.
     *********************************************************************/
    void UpdateRopeTransform(Entity rope) const;

    /*****************************************************************//**
     * \brief   Finds the fixed coordinate where the rope line anchors distantly.
     *********************************************************************/
    Vector2D CalculateFixedAnchor(const Vector2D& originalPosition,
        float halfWidth,
        float halfHeight,
        int ropeIndex) const;

    /*****************************************************************//**
     * \brief   Finds the local coordinate where the rope meets the dynamic parent.
     *********************************************************************/
    Vector2D CalculateDynamicAnchor(const Vector2D& currentPosition,
        float halfWidth,
        float halfHeight,
        int ropeIndex) const;

    /*****************************************************************//**
     * \brief   Raycasts upward looking for collision blocks to anchor.
     *********************************************************************/
    float FindCeilingAnchorY(EntityManager& entityMgr, float attachX, float minY, float fallbackY) const;

    /*****************************************************************//**
     * \brief   Raycasts horizontally looking for collision blocks to anchor.
     *********************************************************************/
    float FindWallAnchorX(EntityManager& entityMgr, float attachY, float maxX, float fallbackX) const;

    Entity platformEntity_ = 0;
    std::vector<Entity> ropeEntities_;
    mutable std::unordered_map<Entity, RopeRuntimeData> ropeRuntimeData_;
    bool hasInitialized_ = false;
    std::string platformType_;
    float originalHalfWidth_ = 40.0f;
    float originalHalfHeight_ = 20.0f;
    Vector2D originalPosition_;
};
