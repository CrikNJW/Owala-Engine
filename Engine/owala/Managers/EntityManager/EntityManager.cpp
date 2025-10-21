/*****************************************************************//**
 * \file	EntityManager.h
 * \brief	Manager responsible for managing entities and their
 *			components in the ECS architecture. Also serves as a
 *			Entity Factory to create entities.
 *
 *
 * \author	Jason Lou
 * \date	26 September 2025
 *********************************************************************/
#include "EntityManager.h"
#include "Logger.h"

namespace Framework {
#pragma region CURRENT ENTITYMANAGER

	// ---------------------------------------------------------------------------
// Constructors
// ---------------------------------------------------------------------------
	EntityManager::EntityManager(ArchetypeManager& archetypemanager, std::unordered_map<Entity, EntityLocation>& entityLocs)
		: archetypeManager(archetypemanager), entityLocations(entityLocs) {}

	// ---------------------------------------------------------------------------
	// EntityManager APIs
	// ---------------------------------------------------------------------------
	Entity EntityManager::CreateEntity() {
		Entity newEntity = nextEntityID++;

		// Place entity into the empty archetype
		ComponentSignature emptySig; // all bits 0
		Archetype* emptyArchetype = archetypeManager.FindOrCreateArchetype(emptySig);
		size_t index = emptyArchetype->entities.size();
		emptyArchetype->entities.push_back(newEntity);

		entityLocations[newEntity] = { emptyArchetype, index };

		return newEntity;
	}

	Entity EntityManager::CloneEntity(Entity& entity) {
		auto it = entityLocations.find(entity);
		if (it == entityLocations.end())
			throw std::runtime_error("Attempted to clone a non-existent entity!");

		EntityLocation& srcLoc = it->second;
		Archetype* srcArchetype = srcLoc.archetype;
		ComponentSignature sig = srcArchetype->signature;

		Entity newEntity = CreateEntity();

		// Add all components from source entity
		for (size_t i = 0; i < MAX_COMPONENTS; ++i) {
			if (!sig.test(i)) continue;

			switch (i) {
			case PlayerStatsType: AddComponent<PlayerStats>(newEntity, GetComponent<PlayerStats>(entity)); break;
			case PositionType: AddComponent<Position>(newEntity, GetComponent<Position>(entity)); break;
			case VelocityType: AddComponent<Velocity>(newEntity, GetComponent<Velocity>(entity)); break;
			case AccelerationType: AddComponent<Acceleration>(newEntity, GetComponent<Acceleration>(entity)); break;
			case TriangleType: AddComponent<Triangle>(newEntity, GetComponent<Triangle>(entity)); break;
			case RigidBodyType: AddComponent<RigidBody>(newEntity, GetComponent<RigidBody>(entity)); break;
			case AABBColliderType: AddComponent<AABBCollider>(newEntity, GetComponent<AABBCollider>(entity)); break;
			case PlayerStateType: AddComponent<PlayerState>(newEntity, GetComponent<PlayerState>(entity)); break;
			case RotationFlagType: AddComponent<RotationFlag>(newEntity, GetComponent<RotationFlag>(entity)); break;
			case TrapPlatformComponentType: AddComponent<TrapPlatformComponent>(newEntity, GetComponent<TrapPlatformComponent>(entity)); break;
			case ScalingPlatformComponentType: AddComponent<ScalingPlatformComponent>(newEntity, GetComponent<ScalingPlatformComponent>(entity)); break;
			case TextureComponentType: AddComponent<TextureComponent>(newEntity, GetComponent<TextureComponent>(entity)); break;
			case AudioComponentType: AddComponent<AudioComponent>(newEntity, GetComponent<AudioComponent>(entity)); break;
			case AnimationComponentType: AddComponent<AnimationComponent>(newEntity, GetComponent<AnimationComponent>(entity)); break;
			case TextComponentType: AddComponent<TextComponent>(newEntity, GetComponent<TextComponent>(entity)); break;
			case CameraType: AddComponent<Camera>(newEntity, GetComponent<Camera>(entity)); break;
			case SceneOwnedType: AddComponent<SceneOwned>(newEntity, GetComponent<SceneOwned>(entity)); break;
			default: break;
			}
		}

		return newEntity;
	}

	bool EntityManager::HasComponent(Entity entity, ComponentType componentType) const {
		auto it = entityLocations.find(entity);
		if (it == entityLocations.end()) return false;
		return it->second.archetype->signature.test(componentType);
	}


	void EntityManager::AddComponent(Entity entity, ComponentType type) {
		if (HasComponent(entity, type)) return;

		switch (type) {
		case PlayerStatsType: AddComponent<PlayerStats>(entity, PlayerStats{}); break;
		case PositionType: AddComponent<Position>(entity, Position{}); break;
		case VelocityType: AddComponent<Velocity>(entity, Velocity{}); break;
		case AccelerationType: AddComponent<Acceleration>(entity, Acceleration{}); break;
		case TriangleType: AddComponent<Triangle>(entity, Triangle{}); break;
		case RigidBodyType: AddComponent<RigidBody>(entity, RigidBody{}); break;
		case AABBColliderType: AddComponent<AABBCollider>(entity, AABBCollider{}); break;
		case PlayerStateType: AddComponent<PlayerState>(entity, PlayerState{}); break;
		case RotationFlagType: AddComponent<RotationFlag>(entity, RotationFlag{}); break;
		case TrapPlatformComponentType: AddComponent<TrapPlatformComponent>(entity, TrapPlatformComponent{}); break;
		case ScalingPlatformComponentType: AddComponent<ScalingPlatformComponent>(entity, ScalingPlatformComponent{}); break;
		case TextureComponentType: AddComponent<TextureComponent>(entity, TextureComponent{}); break;
		case AudioComponentType: AddComponent<AudioComponent>(entity, AudioComponent{}); break;
		case AnimationComponentType: AddComponent<AnimationComponent>(entity, AnimationComponent{}); break;
		case TextComponentType: AddComponent<TextComponent>(entity, TextComponent{}); break;
		case CameraType: AddComponent<Camera>(entity, Camera{}); break;
		case SceneOwnedType: AddComponent<SceneOwned>(entity, SceneOwned{}); break;
		case PlayerTagType:
		case PlatformTagType:
		case BackgroundTagType:
		case EnemyTagType:
		case PersistentTagType:
		case ActiveTagType:
			// Tags are empty and handled via signature only
			break;
		default:
			Logger::Get().Error(DebugType::GameManager) << "AddComponent: Unsupported ComponentType " << static_cast<int>(type);
			return;
		}
	}

	void EntityManager::RemoveComponent(Entity entity, ComponentType type) {
		if (!HasComponent(entity, type)) return;

		EntityLocation& oldLoc = entityLocations.at(entity);
		Archetype* oldArchetype = oldLoc.archetype;
		ComponentSignature newSig = oldArchetype->signature;
		newSig.set(type, false);

		Archetype* newArchetype = archetypeManager.FindOrCreateArchetype(newSig);
		size_t newIndex = newArchetype->entities.size();
		newArchetype->entities.push_back(entity);

		// Move all remaining components
		for (size_t i = 0; i < MAX_COMPONENTS; ++i) {
			if (i == type || !oldArchetype->signature.test(i)) continue;
			auto* oldArray = oldArchetype->componentArrays[i].get();
			auto* newArray = newArchetype->componentArrays[i].get();
			if (oldArray && newArray) oldArray->MoveTo(*newArray, oldLoc.index);
		}

		oldArchetype->RemoveEntity(oldLoc.index, entityLocations);

		// Update location
		entityLocations[entity] = { newArchetype, newIndex };
	}

	bool EntityManager::IsTagComponent(ComponentType componentType) {
		switch (componentType)
		{
		case ComponentType::PlayerTagType:
		case ComponentType::PlatformTagType:
		case ComponentType::BackgroundTagType:
		case ComponentType::EnemyTagType:
		case ComponentType::PersistentTagType:
		case ComponentType::ActiveTagType:
			return true;
		default:
			return false;
		}
	}

	std::vector<Entity> EntityManager::GetAllEntities() const {
		std::vector<Entity> all;
		all.reserve(entityLocations.size());
		for (const auto& [entity, loc] : entityLocations)
			all.push_back(entity);
		return all;
	}

	std::vector<Entity> EntityManager::GetAllEntitiesWithComponent(ComponentType componentType) const {
		std::vector<Entity> entities;

		for (auto& [sig, archetype] : archetypeManager.archetypes) {
			if (archetype->signature.test(componentType)) {
				entities.insert(entities.end(), archetype->entities.begin(), archetype->entities.end());
			}
		}

		return entities;
	}

	ArchetypeManager& EntityManager::GetArchetypeManager() { return archetypeManager; }



#pragma endregion
}