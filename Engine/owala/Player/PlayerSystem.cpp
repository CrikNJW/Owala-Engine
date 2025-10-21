/*****************************************************************//**
 * \file   PlayerSystem.cpp
 *
 * \primary_author
 *      Ng Jing Wen (50%)
 * \secondary_author
 *		Lur Pei Hao Stanley (50%)
 *
 * \brief	Contains Implementations of a Player System that
 *			updates the player entity's components.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.   
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "EntityStorage.h"
#include "PlayerSystem.h"
#include <iostream>
#include "WindowsSystem.h" // Include for glfwGetKey
#include <GLFW/glfw3.h>   // Include for key codes
#include "InputSystem.h" // Include for Input System
#include "PhysicsSystem.h"
#include "FileUtility.h" // Include to write player data on Shutdown
#include "EntityManager.h"

namespace Framework {

	void PlayerSystem::Update(float dt) {
		(void)dt; //Suppress unused parameter warning if not used
		const float moveSpeed = 2.0f; // movement speed
        const float JumpHeight = 0.8f; // jump height
        // calculate jump vel based on height and gravity -  v = sqrt(2 * g * h).
        const float gravity = std::abs(physics_->gravity_.y);
        const float jumpVelocity = std::sqrt(2.0f * gravity * JumpHeight);

		//EntityManager entityMgr(*entityStorage_); // To access entity creation/destruction if needed

        archetypeManager_->ForEachArchetypeWith<Velocity, PlayerState, AnimationComponent, PlayerTag, ActiveTag>(
            [&](Archetype* archetype) {
                // Get component arrays once per archetype
                auto* velocities = archetype->GetComponentArray<Velocity>();
                auto* playerStates = archetype->GetComponentArray<PlayerState>();
                auto* animationComponents = archetype->GetComponentArray<AnimationComponent>();

                // Cache-friendly iteration through all player entities in this archetype
                for (size_t i = 0; i < archetype->entities.size(); ++i) {
                    Velocity& vel = velocities->data[i];
                    PlayerState& state = playerStates->data[i];
                    AnimationComponent& animComp = animationComponents->data[i];

                    // --- Horizontal Movement ---
                    float targetVelX = 0.0f;
                    if (input_->KeyDown(InputSystem::Key::A)) targetVelX = -moveSpeed;
                    if (input_->KeyDown(InputSystem::Key::D)) targetVelX = moveSpeed;
                    vel.Vel.x = targetVelX;

                    // --- Jumping ---
                    if (state.isGrounded &&
                        (input_->KeyPressed(InputSystem::Key::W) ||
                            input_->KeyPressed(InputSystem::Key::Space))) {
                        vel.Vel.y = jumpVelocity;
                    }

                    // --- Animation State Handling ---
                    AnimationState animState = AnimationState::IDLE;

                    if (state.isGrounded) {
                        animState = (std::abs(vel.Vel.x) > 0.01f) ?
                            AnimationState::RUNNING : AnimationState::IDLE;
                    }
                    else {
                        animState = (vel.Vel.y > 0.01f) ?
                            AnimationState::JUMPING : AnimationState::FALLING;
                    }

                    animComp.currentState = animState;
                }
            });

		//std::vector<Archetype*> archetypes = archetypeManager_->GetArchetypesWith<Velocity, PlayerState, AnimationComponent, PlayerTag, ActiveTag>();

		//for (Archetype* archetype : archetypes) {
		//	// Get references to the entities and their component arrays
		//	std::vector<Entity>& entities = archetype->entities;
		//	auto* velocities = archetype->GetComponentArray<Velocity>();
		//	auto* playerStates = archetype->GetComponentArray<PlayerState>();
		//	auto* animationComponents = archetype->GetComponentArray<AnimationComponent>();

  //          for (size_t i = 0; i < archetype->entities.size(); ++i) {
		//		Velocity& vel = velocities->data[i];
		//		PlayerState& state = playerStates->data[i];
		//		AnimationComponent& animationComponent = animationComponents->data[i];

  //              // --- Horizontal Movement ---
  //              float targetVelX = 0.0f;
  //              if (input_->KeyDown(InputSystem::Key::A)) targetVelX = -moveSpeed;
  //              if (input_->KeyDown(InputSystem::Key::D)) targetVelX = moveSpeed;
  //              vel.Vel.x = targetVelX;

  //              // --- Jumping ---
  //              if (state.isGrounded && (input_->KeyPressed(InputSystem::Key::W) || input_->KeyPressed(InputSystem::Key::Space))) {
  //                  vel.Vel.y = jumpVelocity; // Apply the calculated upward velocity

  //              }

  //              // Animation State Handling
  //              AnimationState animState = AnimationState::IDLE; // Default to IDLE

  //              if (state.isGrounded) {
  //                  if (std::abs(vel.Vel.x) > 0.01f) { animState = AnimationState::RUNNING; } //If player is on ground and moving, set to running
  //                  else { animState = AnimationState::IDLE; } //If player is on ground and not moving, set to idle
  //              }
  //              else {
  //                  if (vel.Vel.y > 0.01f) { animState = AnimationState::JUMPING; } //If player is in air and moving upwards, set to jumping
  //                  else if (vel.Vel.y <= 0.0f) { animState = AnimationState::FALLING; } //If player is in air, at the point he is beginning to fall down or moving downwards, set to falling
  //              }

  //              animationComponent.currentState = animState;
  //          }

		//}

		//// Loop through each entity that is in the playerTag map
		//for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(PlayerTagType)) {
  //          // Check if Player Entity is active in the scene. Handle player controller if active.
  //          if (!entityMgr.HasComponent(entity, ActiveTagType)) continue;

		//	

  //          // We need a RigidBody, Velocity, and PlayerState to control the player
  //          if (entityMgr.HasComponent(entity, VelocityType) && entityMgr.HasComponent(entity, PlayerStateType) && entityMgr.HasComponent(entity, AnimationComponentType)) {
  //              Velocity& vel = entityMgr.GetComponent<Velocity>(entity);
  //              PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);
  //              AnimationComponent& animationComponent = entityMgr.GetComponent<AnimationComponent>(entity);

  //              // --- Horizontal Movement ---
  //              float targetVelX = 0.0f;
  //              if (input_->KeyDown(InputSystem::Key::A)) targetVelX = -moveSpeed;
  //              if (input_->KeyDown(InputSystem::Key::D)) targetVelX = moveSpeed;
  //              vel.Vel.x = targetVelX;

  //              // --- Jumping ---
  //              if (state.isGrounded && (input_->KeyPressed(InputSystem::Key::W) || input_->KeyPressed(InputSystem::Key::Space))) {
  //                  vel.Vel.y = jumpVelocity; // Apply the calculated upward velocity
  //                  
  //              }

  //              // Animation State Handling
		//		AnimationState animState = AnimationState::IDLE; // Default to IDLE

		//		if (state.isGrounded) {
  //                  if (std::abs(vel.Vel.x) > 0.01f) { animState = AnimationState::RUNNING; } //If player is on ground and moving, set to running
  //                  else { animState = AnimationState::IDLE; } //If player is on ground and not moving, set to idle
		//		}
		//		else {
  //                  if (vel.Vel.y > 0.01f) { animState = AnimationState::JUMPING; } //If player is in air and moving upwards, set to jumping
  //                  else if (vel.Vel.y <= 0.0f) { animState = AnimationState::FALLING; } //If player is in air, at the point he is beginning to fall down or moving downwards, set to falling
		//		}

		//		animationComponent.currentState = animState;
  //          }
  //      }
    }


    void PlayerSystem::Shutdown() {
        //Logger::Get().Info(DebugType::PlayerSystem) << "Player system shutdown" << std::endl;

        /*FileIOSystem fileIO;
        PlayerStats newStats;
        EntityManager entityMgr(*entityStorage_);

        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(PlayerTagType)) {
            newStats = entityMgr.GetComponent<PlayerStats>(entity);
            newStats.pos = entityMgr.GetComponent<Position>(entity);
        } */

        //logger.Info(DebugType::PlayerSystem) << "Writing Player Stats" << std::endl;
        //fileIO.WritePlayerStats(newStats);
    }

} // namespace Framework
