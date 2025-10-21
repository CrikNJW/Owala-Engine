/*****************************************************************//**
 * \file   AnimationSystem.cpp
 * \brief
 *
 *
 * \author Ng Jing Wen
 * \date   10 September 2025
 *********************************************************************/
#include "EntityStorage.h"
#include "EntityManager.h"
#include "AnimationSystem.h"
#include <iostream>
#include "WindowsSystem.h" // Include for glfwGetKey
#include <GLFW/glfw3.h>   // Include for key codes

namespace Framework {

    void AnimationSystem::Update(float dt) {
        // Iterate archetypes with AnimationComponent and TextureComponent
        entityManager_.GetArchetypeManager().ForEachArchetypeWith<AnimationComponent, TextureComponent>(
            [&](Archetype* archetype) {
                auto* animations = archetype->GetComponentArray<AnimationComponent>();
                auto* textures = archetype->GetComponentArray<TextureComponent>();

                // Cache-friendly iteration through all entities in this archetype
                for (size_t i = 0; i < archetype->entities.size(); ++i) {
                    AnimationComponent& animComp = animations->data[i];
                    TextureComponent& texComp = textures->data[i];

                    // Set texture key based on animation state
                    switch (animComp.currentState) {
                    case AnimationState::IDLE:    texComp.textureKey = "playerSprite_Idle"; break;
                    case AnimationState::RUNNING: texComp.textureKey = "playerSprite_Run"; break;
                    case AnimationState::JUMPING: texComp.textureKey = "playerSprite_Jump"; break;
                    case AnimationState::FALLING: texComp.textureKey = "playerSprite_Jump"; break;
                    }

                    // Retrieve animation info
                    AnimationInfo& animInfo = resourceManager_->get<AnimationInfo>(texComp.textureKey);

                    // Update animation component properties
                    animComp.frameCount = animInfo.frameCount;
                    animComp.frameDuration = animInfo.frameDuration;
                    animComp.isLooping = animInfo.isLooping;
                    animComp.startFrame = animInfo.startFrame;

                    // Check if animation is playing
                    if (!animComp.isPlaying) continue;

                    // Update elapsed time
                    animComp.elapsedTime += dt;

                    // Check if it's time to switch to the next frame
                    if (animComp.elapsedTime >= animComp.frameDuration) {
                        animComp.elapsedTime -= animComp.frameDuration;
                        animComp.currentFrame++;

                        // Handle looping
                        if (animComp.currentFrame >= animComp.frameCount) {
                            if (animComp.isLooping) {
                                animComp.currentFrame = 0;
                            }
                            else {
                                animComp.currentFrame = animComp.frameCount - 1;
                                animComp.isPlaying = false;
                            }
                        }
                    }

                    // Calculate UV coordinates
                    if (animComp.frameCount > 0) {
                        float frameUVWidth = 1.0f / static_cast<float>(animComp.frameCount);
                        animComp.minUV.x = static_cast<float>(animComp.currentFrame) * frameUVWidth;
                        animComp.maxUV.x = animComp.minUV.x + frameUVWidth;
                        animComp.minUV.y = 0.0f;
                        animComp.maxUV.y = 1.0f;
                    }
                }
            });
    }

	void AnimationSystem::Shutdown() {
	}


} // namespace Framework
