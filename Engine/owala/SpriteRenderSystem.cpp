#include "SpriteRenderSystem.h"
#include "EntityManager.h"
#include "CameraSystem.h"
#include "Matrix3x3.h"
#include <cmath>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Framework {

    struct SpriteBatch {
        Texture* texture = nullptr;             // Texture for this batch
        std::vector<SpriteVertex> vertices;     // Vertex buffer (4 vertices per sprite)
        std::vector<uint32_t> indices;          // Index buffer (6 indices per sprite)
    };

    void SpriteRenderSystem::Update(float /*dt*/) {
        // Clear frame once per frame (do NOT also clear in GameSystem anymore)
        gfx_.beginFrame(clearColor_);
        RenderSprites();
        if (drawColliders_) {
            RenderColliderOutlines();
        }
        // Note: your Graphics system (added last) should still be the one that swaps buffers.
    }

    void SpriteRenderSystem::RenderSprites() {
        Entity activeCamera = 0;
        if (cameraSystem_)
            activeCamera = cameraSystem_->GetActiveCamera();

        float left = -1.0f, right = 1.0f, bottom = -1.0f, top = 1.0f;
        if (activeCamera != 0 && cameraSystem_)
            cameraSystem_->GetViewBounds(activeCamera, left, right, bottom, top);

        std::unordered_map<Texture*, SpriteBatch> batches;

        // Iterate all archetypes with TextureComponent, Position, ActiveTag
        archetypeManager_.ForEachArchetypeWith<TextureComponent, Position, ActiveTag>(
            [&](Archetype* archetype) {

                auto* textures = archetype->GetComponentArray<TextureComponent>();
                auto* positions = archetype->GetComponentArray<Position>();
                auto* colliders = archetype->GetComponentArray<AABBCollider>();
                auto* rotations = archetype->GetComponentArray<RotationFlag>();
                auto* animations = archetype->GetComponentArray<AnimationComponent>();

                for (size_t i = 0; i < archetype->entities.size(); ++i) {
                    TextureComponent& tex = textures->data[i];
                    Position& pos = positions->data[i];
                    Vector2D worldPos = pos.Pos;

                    Vector2D halfSize = colliders ? colliders->data[i].HalfSize : tex.HalfSize;

                    // Cull out-of-camera sprites
                    if (activeCamera != 0 && cameraSystem_) {
                        if (worldPos.x + halfSize.x < left || worldPos.x - halfSize.x > right ||
                            worldPos.y + halfSize.y < bottom || worldPos.y - halfSize.y > top)
                            continue;
                    }

                    float rotationRad = rotations ? rotations->data[i].currentRotation * (M_PI / 180.0f) : 0.0f;

                    Texture& texture = resources_.get<Texture>(tex.textureKey);

                    Vector2D screenPos = worldPos;
                    Vector2D screenHalfSize = halfSize;

                    if (activeCamera != 0 && cameraSystem_) {
                        screenPos = cameraSystem_->WorldToScreen(worldPos, activeCamera);
                        const Vector2D sx = cameraSystem_->WorldToScreen(worldPos + Vector2D{ 1.0f, 0.0f }, activeCamera) - screenPos;
                        const Vector2D sy = cameraSystem_->WorldToScreen(worldPos + Vector2D{ 0.0f, 1.0f }, activeCamera) - screenPos;
                        screenHalfSize.x = std::abs(sx.x) * halfSize.x;
                        screenHalfSize.y = std::abs(sy.y) * halfSize.y;
                    }

                    // Create a batch for this texture if needed
                    SpriteBatch& batch = batches[&texture];

                    batch.texture = &texture;

                    // Compute vertices (rotated quad)
                    Vector2D corners[4] = {
                        {-screenHalfSize.x, -screenHalfSize.y},
                        { screenHalfSize.x, -screenHalfSize.y},
                        { screenHalfSize.x,  screenHalfSize.y},
                        {-screenHalfSize.x,  screenHalfSize.y}
                    };

                    // Apply rotation
                    if (std::abs(rotationRad) > 0.001f) {
                        float c = std::cos(rotationRad);
                        float s = std::sin(rotationRad);
                        for (int j = 0; j < 4; ++j) {
                            float x = corners[j].x;
                            float y = corners[j].y;
                            corners[j].x = x * c - y * s;
                            corners[j].y = x * s + y * c;
                        }
                    }

                    // Translate to screen position
                    for (int j = 0; j < 4; ++j) {
                        corners[j] = corners[j] + screenPos;
                    }

                    // UV coordinates
                    Vector2D minUV{ 0,0 }, maxUV{ 1,1 };
                    if (animations) {
                        const AnimationComponent& anim = animations->data[i];
                        minUV = anim.minUV;
                        maxUV = anim.maxUV;
                    }

                    // Append vertices
                    uint32_t startIndex = static_cast<uint32_t>(batch.vertices.size());
                    batch.vertices.push_back({ Vector2D{corners[0].x, corners[0].y}, Vector2D{minUV.x, minUV.y}, Color{1,1,1,1} });
                    batch.vertices.push_back({ Vector2D{corners[1].x, corners[1].y}, Vector2D{maxUV.x, minUV.y}, Color{1,1,1,1} });
                    batch.vertices.push_back({ Vector2D{corners[2].x, corners[2].y}, Vector2D{maxUV.x, maxUV.y}, Color{1,1,1,} });
                    batch.vertices.push_back({ Vector2D{corners[3].x, corners[3].y}, Vector2D{minUV.x, maxUV.y}, Color{1,1,1,1} });

                    // Append indices
                    batch.indices.push_back(startIndex + 0);
                    batch.indices.push_back(startIndex + 1);
                    batch.indices.push_back(startIndex + 2);
                    batch.indices.push_back(startIndex + 2);
                    batch.indices.push_back(startIndex + 3);
                    batch.indices.push_back(startIndex + 0);
                }
            });

        // === FLUSH BATCHES ===
        for (auto& [texPtr, batch] : batches) {
            gfx_.drawSpriteBatch(batch.vertices, batch.indices, *batch.texture);
        }

        archetypeManager_.ForEachArchetypeWith<TextComponent, Position>(
            [&](Archetype* archetype) {
                auto* texts = archetype->GetComponentArray<TextComponent>();
                auto* positions = archetype->GetComponentArray<Position>();

                for (size_t i = 0; i < archetype->entities.size(); ++i) {
                    const TextComponent& component = texts->data[i];
                    const Vector2D& worldTextPos = positions->data[i].Pos;

                    Font& font = resources_.get<Font>(component.fontName);

                    Vector2D screenTextPos = worldTextPos;
                    if (activeCamera != 0 && cameraSystem_) {
                        screenTextPos = cameraSystem_->WorldToScreen(worldTextPos, activeCamera);
                    }

                    const float max_width_ndc = 1.8f;
                    const float max_width_pixels = max_width_ndc * 0.5f * gfx_.getScreenWidth();
                    std::vector<std::string> lines = gfx_.WrapText(font, component.textToDraw, component.scale, max_width_pixels);

                    Vector2D current_pos = screenTextPos;
                    const float line_height_ndc = 0.08f;
                    for (const auto& line : lines) {
                        gfx_.drawText(font, line, current_pos, component.scale, component.color);
                        current_pos.y -= line_height_ndc;
                    }
                }
            });
    }


    void SpriteRenderSystem::RenderColliderOutlines() {
        EntityManager entityMgr(ecs_);

        // === GET ACTIVE CAMERA ===
        Entity activeCamera = 0;
        if (cameraSystem_) {
            activeCamera = cameraSystem_->GetActiveCamera();
        }

        archetypeManager_.ForEachArchetypeWith<AABBCollider, Position>(
            [&](Archetype* archetype) {
                auto* colliders = archetype->GetComponentArray<AABBCollider>();
                auto* positions = archetype->GetComponentArray<Position>();
                auto* rotations = archetype->GetComponentArray<RotationFlag>();

                for (size_t i = 0; i < archetype->entities.size(); ++i) {
                    AABBCollider& collider = colliders->data[i];
                    Position& pos = positions->data[i];

                    const Vector2D& worldPos = pos.Pos;
                    const Vector2D& halfSize = collider.HalfSize;

                    // WORLD -> SCREEN
                    Vector2D screenPos = worldPos;
                    Vector2D screenHalfSize = halfSize;

                    if (activeCamera != 0 && cameraSystem_) {
                        screenPos = cameraSystem_->WorldToScreen(worldPos, activeCamera);

                        const Vector2D sx = cameraSystem_->WorldToScreen(worldPos + Vector2D{ 1.0f, 0.0f }, activeCamera) - screenPos;
                        const Vector2D sy = cameraSystem_->WorldToScreen(worldPos + Vector2D{ 0.0f, 1.0f }, activeCamera) - screenPos;

                        screenHalfSize.x = std::abs(sx.x) * halfSize.x;
                        screenHalfSize.y = std::abs(sy.y) * halfSize.y;
                    }

                    float deg = 0.0f;
                    if (rotations) {
                        RotationFlag& rf = rotations->data[i];
                        deg = rf.currentRotation;
                    }

                    if (std::abs(deg) < 0.001f) {
                        // Non-rotated AABB in screen space
                        Vector2D corners[4] = {
                            {screenPos.x - screenHalfSize.x, screenPos.y - screenHalfSize.y},
                            {screenPos.x + screenHalfSize.x, screenPos.y - screenHalfSize.y},
                            {screenPos.x + screenHalfSize.x, screenPos.y + screenHalfSize.y},
                            {screenPos.x - screenHalfSize.x, screenPos.y + screenHalfSize.y}
                        };
                        for (int i = 0; i < 4; ++i) {
                            int next = (i + 1) % 4;
                            gfx_.drawLine(corners[i], corners[next], { 0.0f, 1.0f, 0.0f, 1.0f });
                        }
                    }
                    else {
                        // Rotated rectangle in screen space
                        const float r = deg * (float)M_PI / 180.0f;
                        const float c = std::cos(r);
                        const float s = std::sin(r);

                        Vector2D local[4] = {
                            {-screenHalfSize.x, -screenHalfSize.y},
                            { screenHalfSize.x, -screenHalfSize.y},
                            { screenHalfSize.x,  screenHalfSize.y},
                            {-screenHalfSize.x,  screenHalfSize.y}
                        };

                        Vector2D corners[4];
                        for (int i = 0; i < 4; ++i) {
                            corners[i].x = local[i].x * c - local[i].y * s + screenPos.x;
                            corners[i].y = local[i].x * s + local[i].y * c + screenPos.y;
                        }

                        for (int i = 0; i < 4; ++i) {
                            int next = (i + 1) % 4;
                            gfx_.drawLine(corners[i], corners[next], { 0.0f, 1.0f, 0.0f, 1.0f });
                        }
                    }
                }
            });
    }

} // namespace Framework