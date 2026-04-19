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
#include "pch.h"
#include "EntityManager.h"
#include "Logger.h"
#include "ScriptRegistry.h"
#include "CSharpScriptEngine.h"
#include <algorithm>

namespace Framework {
#pragma region CURRENT ENTITYMANAGER

	// ---------------------------------------------------------------------------
	// Constructors
	// ---------------------------------------------------------------------------
	EntityManager::EntityManager(EntityStorage& entityStorage) : ecs(entityStorage) {};

	std::size_t EntityManager::GetOrCreateArchetype(const ComponentSignature& signature) {
		auto it = ecs.signatureToArchetype.find(signature);
		if (it != ecs.signatureToArchetype.end()) {
			return it->second;
		}

		const std::size_t archetypeIndex = ecs.archetypes.size();
		ecs.archetypes.push_back({ signature, {} });
		ecs.signatureToArchetype[signature] = archetypeIndex;
		return archetypeIndex;
	}

	void EntityManager::ReserveComponentSlotsForEntity(ArchetypeBucket& bucket, const ComponentSignature& signature) {
		if (signature.test(TransformType)) bucket.transform.emplace_back();
		if (signature.test(VelocityType)) bucket.velocity.emplace_back();
		if (signature.test(AccelerationType)) bucket.acceleration.emplace_back();
		if (signature.test(TriangleType)) bucket.triangle.emplace_back();
		if (signature.test(RigidBodyType)) bucket.rigidBody.emplace_back();
		if (signature.test(AABBColliderType)) bucket.colliders.emplace_back();
		if (signature.test(PlayerStateType)) bucket.playerState.emplace_back();
		if (signature.test(RotationFlagType)) bucket.rotationFlag.emplace_back();
		if (signature.test(TrapPlatformComponentType)) bucket.trapPlatform.emplace_back();
		if (signature.test(ButtonType)) bucket.button.emplace_back();
		if (signature.test(TextureComponentType)) bucket.tex.emplace_back();
		if (signature.test(AudioComponentType)) bucket.audio.emplace_back();
		if (signature.test(AnimationComponentType)) bucket.animatedTex.emplace_back();
		if (signature.test(TextComponentType)) bucket.text.emplace_back();
		if (signature.test(CameraType)) bucket.cameras.emplace_back();
		if (signature.test(Transform3DType)) bucket.transform3D.emplace_back();
		if (signature.test(Primitive3DComponentType)) bucket.primitive3D.emplace_back();
		if (signature.test(DirectionalLight3DType)) bucket.directionalLights3D.emplace_back();
		if (signature.test(NativeScriptsComponentType)) bucket.nativeScripts.emplace_back();
		if (signature.test(SceneOwnedType)) bucket.sceneOwned.emplace_back();
		if (signature.test(FSMComponentType)) bucket.fsmComponent.emplace_back();
		if (signature.test(ParticleComponentType)) bucket.particleComponent.emplace_back();
		if (signature.test(ParticleEmitterComponentType)) bucket.particleEmitterComponent.emplace_back();
		if (signature.test(SliderType)) bucket.sliders.emplace_back();
		if (signature.test(PlayerHealthComponentType)) bucket.playerHealth.emplace_back();
	}

	void EntityManager::CopySharedComponents(ArchetypeBucket& source, std::size_t sourceRow, ArchetypeBucket& target, const ComponentSignature& sharedSignature) {
		if (sharedSignature.test(TransformType)) target.transform.back() = source.transform[sourceRow];
		if (sharedSignature.test(VelocityType)) target.velocity.back() = source.velocity[sourceRow];
		if (sharedSignature.test(AccelerationType)) target.acceleration.back() = source.acceleration[sourceRow];
		if (sharedSignature.test(TriangleType)) target.triangle.back() = source.triangle[sourceRow];
		if (sharedSignature.test(RigidBodyType)) target.rigidBody.back() = source.rigidBody[sourceRow];
		if (sharedSignature.test(AABBColliderType)) target.colliders.back() = source.colliders[sourceRow];
		if (sharedSignature.test(PlayerStateType)) target.playerState.back() = source.playerState[sourceRow];
		if (sharedSignature.test(RotationFlagType)) target.rotationFlag.back() = source.rotationFlag[sourceRow];
		if (sharedSignature.test(TrapPlatformComponentType)) target.trapPlatform.back() = source.trapPlatform[sourceRow];
		if (sharedSignature.test(ButtonType)) target.button.back() = source.button[sourceRow];
		if (sharedSignature.test(TextureComponentType)) target.tex.back() = source.tex[sourceRow];
		if (sharedSignature.test(AudioComponentType)) target.audio.back() = source.audio[sourceRow];
		if (sharedSignature.test(AnimationComponentType)) target.animatedTex.back() = source.animatedTex[sourceRow];
		if (sharedSignature.test(TextComponentType)) target.text.back() = source.text[sourceRow];
		if (sharedSignature.test(CameraType)) target.cameras.back() = source.cameras[sourceRow];
		if (sharedSignature.test(Transform3DType)) target.transform3D.back() = source.transform3D[sourceRow];
		if (sharedSignature.test(Primitive3DComponentType)) target.primitive3D.back() = source.primitive3D[sourceRow];
		if (sharedSignature.test(DirectionalLight3DType)) target.directionalLights3D.back() = source.directionalLights3D[sourceRow];
		if (sharedSignature.test(NativeScriptsComponentType)) target.nativeScripts.back() = std::move(source.nativeScripts[sourceRow]);
		if (sharedSignature.test(SceneOwnedType)) target.sceneOwned.back() = source.sceneOwned[sourceRow];
		if (sharedSignature.test(FSMComponentType)) target.fsmComponent.back() = source.fsmComponent[sourceRow];
		if (sharedSignature.test(ParticleComponentType)) target.particleComponent.back() = source.particleComponent[sourceRow];
		if (sharedSignature.test(ParticleEmitterComponentType)) target.particleEmitterComponent.back() = source.particleEmitterComponent[sourceRow];
		if (sharedSignature.test(SliderType)) target.sliders.back() = source.sliders[sourceRow];
		if (sharedSignature.test(PlayerHealthComponentType)) target.playerHealth.back() = source.playerHealth[sourceRow];
	}

	void EntityManager::AddComponentToBucket(ArchetypeBucket& bucket, ComponentType componentType, const void* componentData) {
		if (!componentData) return;

		switch (componentType) {
		case TransformType: bucket.transform.back() = *static_cast<const Transform*>(componentData); break;
		case VelocityType: bucket.velocity.back() = *static_cast<const Velocity*>(componentData); break;
		case AccelerationType: bucket.acceleration.back() = *static_cast<const Acceleration*>(componentData); break;
		case TriangleType: bucket.triangle.back() = *static_cast<const Triangle*>(componentData); break;
		case RigidBodyType: bucket.rigidBody.back() = *static_cast<const RigidBody*>(componentData); break;
		case AABBColliderType: bucket.colliders.back() = *static_cast<const AABBCollider*>(componentData); break;
		case PlayerStateType: bucket.playerState.back() = *static_cast<const PlayerState*>(componentData); break;
		case RotationFlagType: bucket.rotationFlag.back() = *static_cast<const RotationFlag*>(componentData); break;
		case TrapPlatformComponentType: bucket.trapPlatform.back() = *static_cast<const TrapPlatformComponent*>(componentData); break;
		case ButtonType: bucket.button.back() = *static_cast<const Button*>(componentData); break;
		case TextureComponentType: bucket.tex.back() = *static_cast<const TextureComponent*>(componentData); break;
		case AudioComponentType: bucket.audio.back() = *static_cast<const AudioComponent*>(componentData); break;
		case AnimationComponentType: bucket.animatedTex.back() = *static_cast<const AnimationComponent*>(componentData); break;
		case TextComponentType: bucket.text.back() = *static_cast<const TextComponent*>(componentData); break;
		case CameraType: bucket.cameras.back() = *static_cast<const Camera*>(componentData); break;
		case Transform3DType: bucket.transform3D.back() = *static_cast<const Transform3D*>(componentData); break;
		case Primitive3DComponentType: bucket.primitive3D.back() = *static_cast<const Primitive3D*>(componentData); break;
		case DirectionalLight3DType: bucket.directionalLights3D.back() = *static_cast<const DirectionalLight3D*>(componentData); break;
		case NativeScriptsComponentType: bucket.nativeScripts.back() = *static_cast<const NativeScriptsComponent*>(componentData); break;
		case SceneOwnedType: bucket.sceneOwned.back() = *static_cast<const SceneOwned*>(componentData); break;
		case FSMComponentType: bucket.fsmComponent.back() = *static_cast<const FSMComponent*>(componentData); break;
		case ParticleComponentType: bucket.particleComponent.back() = *static_cast<const ParticleComponent*>(componentData); break;
		case ParticleEmitterComponentType: bucket.particleEmitterComponent.back() = *static_cast<const ParticleEmitterComponent*>(componentData); break;
		case SliderType: bucket.sliders.back() = *static_cast<const Slider*>(componentData); break;
		case PlayerHealthComponentType: bucket.playerHealth.back() = *static_cast<const PlayerHealthComponent*>(componentData); break;
		default: break;
		}
	}

	void EntityManager::RemoveComponentFromBucket(ArchetypeBucket& bucket, ComponentType componentType, std::size_t rowIndex) {
		if (componentType == NativeScriptsComponentType && rowIndex < bucket.nativeScripts.size()) {
			NativeScriptsComponent& scripts = bucket.nativeScripts[rowIndex];
			const Entity owningEntity = (rowIndex < bucket.entities.size()) ? bucket.entities[rowIndex] : INVALID_ENTITY;
			for (NativeScriptsComponent::NativeScript& script : scripts.scripts) {
				if (owningEntity != INVALID_ENTITY) {
					CSharpScriptEngine::Get().DestroyScriptInstance(owningEntity, script);
				}
				script.Reset();
			}
		}
	}

	void EntityManager::EraseRowFromArchetype(std::size_t archetypeIndex, std::size_t rowIndex) {
		ArchetypeBucket& bucket = ecs.archetypes[archetypeIndex];
		const std::size_t lastRow = bucket.entities.size() - 1;

		if (rowIndex != lastRow) {
			const Entity swappedEntity = bucket.entities[lastRow];
			bucket.entities[rowIndex] = swappedEntity;
			ecs.entityToArchetypeSlot[swappedEntity] = rowIndex;

			if (bucket.signature.test(TransformType)) bucket.transform[rowIndex] = std::move(bucket.transform[lastRow]);
			if (bucket.signature.test(VelocityType)) bucket.velocity[rowIndex] = std::move(bucket.velocity[lastRow]);
			if (bucket.signature.test(AccelerationType)) bucket.acceleration[rowIndex] = std::move(bucket.acceleration[lastRow]);
			if (bucket.signature.test(TriangleType)) bucket.triangle[rowIndex] = std::move(bucket.triangle[lastRow]);
			if (bucket.signature.test(RigidBodyType)) bucket.rigidBody[rowIndex] = std::move(bucket.rigidBody[lastRow]);
			if (bucket.signature.test(AABBColliderType)) bucket.colliders[rowIndex] = std::move(bucket.colliders[lastRow]);
			if (bucket.signature.test(PlayerStateType)) bucket.playerState[rowIndex] = std::move(bucket.playerState[lastRow]);
			if (bucket.signature.test(RotationFlagType)) bucket.rotationFlag[rowIndex] = std::move(bucket.rotationFlag[lastRow]);
			if (bucket.signature.test(TrapPlatformComponentType)) bucket.trapPlatform[rowIndex] = std::move(bucket.trapPlatform[lastRow]);
			if (bucket.signature.test(ButtonType)) bucket.button[rowIndex] = std::move(bucket.button[lastRow]);
			if (bucket.signature.test(TextureComponentType)) bucket.tex[rowIndex] = std::move(bucket.tex[lastRow]);
			if (bucket.signature.test(AudioComponentType)) bucket.audio[rowIndex] = std::move(bucket.audio[lastRow]);
			if (bucket.signature.test(AnimationComponentType)) bucket.animatedTex[rowIndex] = std::move(bucket.animatedTex[lastRow]);
			if (bucket.signature.test(TextComponentType)) bucket.text[rowIndex] = std::move(bucket.text[lastRow]);
			if (bucket.signature.test(CameraType)) bucket.cameras[rowIndex] = std::move(bucket.cameras[lastRow]);
			if (bucket.signature.test(Transform3DType)) bucket.transform3D[rowIndex] = std::move(bucket.transform3D[lastRow]);
			if (bucket.signature.test(Primitive3DComponentType)) bucket.primitive3D[rowIndex] = std::move(bucket.primitive3D[lastRow]);
			if (bucket.signature.test(DirectionalLight3DType)) bucket.directionalLights3D[rowIndex] = std::move(bucket.directionalLights3D[lastRow]);
			if (bucket.signature.test(NativeScriptsComponentType)) bucket.nativeScripts[rowIndex] = std::move(bucket.nativeScripts[lastRow]);
			if (bucket.signature.test(SceneOwnedType)) bucket.sceneOwned[rowIndex] = std::move(bucket.sceneOwned[lastRow]);
			if (bucket.signature.test(FSMComponentType)) bucket.fsmComponent[rowIndex] = std::move(bucket.fsmComponent[lastRow]);
			if (bucket.signature.test(ParticleComponentType)) bucket.particleComponent[rowIndex] = std::move(bucket.particleComponent[lastRow]);
			if (bucket.signature.test(ParticleEmitterComponentType)) bucket.particleEmitterComponent[rowIndex] = std::move(bucket.particleEmitterComponent[lastRow]);
			if (bucket.signature.test(SliderType)) bucket.sliders[rowIndex] = std::move(bucket.sliders[lastRow]);
			if (bucket.signature.test(PlayerHealthComponentType)) bucket.playerHealth[rowIndex] = std::move(bucket.playerHealth[lastRow]);
		}

		bucket.entities.pop_back();
		if (bucket.signature.test(TransformType)) bucket.transform.pop_back();
		if (bucket.signature.test(VelocityType)) bucket.velocity.pop_back();
		if (bucket.signature.test(AccelerationType)) bucket.acceleration.pop_back();
		if (bucket.signature.test(TriangleType)) bucket.triangle.pop_back();
		if (bucket.signature.test(RigidBodyType)) bucket.rigidBody.pop_back();
		if (bucket.signature.test(AABBColliderType)) bucket.colliders.pop_back();
		if (bucket.signature.test(PlayerStateType)) bucket.playerState.pop_back();
		if (bucket.signature.test(RotationFlagType)) bucket.rotationFlag.pop_back();
		if (bucket.signature.test(TrapPlatformComponentType)) bucket.trapPlatform.pop_back();
		if (bucket.signature.test(ButtonType)) bucket.button.pop_back();
		if (bucket.signature.test(TextureComponentType)) bucket.tex.pop_back();
		if (bucket.signature.test(AudioComponentType)) bucket.audio.pop_back();
		if (bucket.signature.test(AnimationComponentType)) bucket.animatedTex.pop_back();
		if (bucket.signature.test(TextComponentType)) bucket.text.pop_back();
		if (bucket.signature.test(CameraType)) bucket.cameras.pop_back();
		if (bucket.signature.test(Transform3DType)) bucket.transform3D.pop_back();
		if (bucket.signature.test(Primitive3DComponentType)) bucket.primitive3D.pop_back();
		if (bucket.signature.test(DirectionalLight3DType)) bucket.directionalLights3D.pop_back();
		if (bucket.signature.test(NativeScriptsComponentType)) bucket.nativeScripts.pop_back();
		if (bucket.signature.test(SceneOwnedType)) bucket.sceneOwned.pop_back();
		if (bucket.signature.test(FSMComponentType)) bucket.fsmComponent.pop_back();
		if (bucket.signature.test(ParticleComponentType)) bucket.particleComponent.pop_back();
		if (bucket.signature.test(ParticleEmitterComponentType)) bucket.particleEmitterComponent.pop_back();
		if (bucket.signature.test(SliderType)) bucket.sliders.pop_back();
		if (bucket.signature.test(PlayerHealthComponentType)) bucket.playerHealth.pop_back();
	}

	void EntityManager::RemoveEntityFromArchetype(Entity entity) {
		auto archetypeIt = ecs.entityToArchetype.find(entity);
		auto slotIt = ecs.entityToArchetypeSlot.find(entity);
		if (archetypeIt == ecs.entityToArchetype.end() || slotIt == ecs.entityToArchetypeSlot.end()) {
			return;
		}

		EraseRowFromArchetype(archetypeIt->second, slotIt->second);
		ecs.entityToArchetype.erase(archetypeIt);
		ecs.entityToArchetypeSlot.erase(slotIt);
	}

	void EntityManager::MoveEntityToArchetype(Entity entity, const ComponentSignature& signature) {
		RemoveEntityFromArchetype(entity);

		const std::size_t archetypeIndex = GetOrCreateArchetype(signature);
		EntityStorage::ArchetypeBucket& targetBucket = ecs.archetypes[archetypeIndex];
		ecs.entityToArchetype[entity] = archetypeIndex;
		ecs.entityToArchetypeSlot[entity] = targetBucket.entities.size();
		targetBucket.entities.push_back(entity);
		ReserveComponentSlotsForEntity(targetBucket, signature);
	}

	void EntityManager::AddComponentInternal(Entity entity, ComponentType componentType, const void* componentData) {
		auto sigIt = ecs.entitySignatures.find(entity);
		if (sigIt == ecs.entitySignatures.end()) return;
		if (sigIt->second.test(componentType)) return;

		const ComponentSignature oldSignature = sigIt->second;
		ComponentSignature newSignature = oldSignature;
		newSignature.set(componentType);

		const std::size_t oldArchetypeIndex = ecs.entityToArchetype.at(entity);
		const std::size_t oldRow = ecs.entityToArchetypeSlot.at(entity);
		const std::size_t newArchetypeIndex = GetOrCreateArchetype(newSignature);

		ArchetypeBucket& oldBucket = ecs.archetypes[oldArchetypeIndex];
		ArchetypeBucket& newBucket = ecs.archetypes[newArchetypeIndex];
		const std::size_t newRow = newBucket.entities.size();

		newBucket.entities.push_back(entity);
		ReserveComponentSlotsForEntity(newBucket, newSignature);
		CopySharedComponents(oldBucket, oldRow, newBucket, oldSignature);
		AddComponentToBucket(newBucket, componentType, componentData);

		EraseRowFromArchetype(oldArchetypeIndex, oldRow);
		ecs.entityToArchetype[entity] = newArchetypeIndex;
		ecs.entityToArchetypeSlot[entity] = newRow;
		sigIt->second = newSignature;
	}

	void EntityManager::RemoveComponentInternal(Entity entity, ComponentType componentType) {
		auto sigIt = ecs.entitySignatures.find(entity);
		if (sigIt == ecs.entitySignatures.end()) return;
		if (!sigIt->second.test(componentType)) return;

		const ComponentSignature oldSignature = sigIt->second;
		ComponentSignature newSignature = oldSignature;
		newSignature.reset(componentType);

		const std::size_t oldArchetypeIndex = ecs.entityToArchetype.at(entity);
		const std::size_t oldRow = ecs.entityToArchetypeSlot.at(entity);
		const std::size_t newArchetypeIndex = GetOrCreateArchetype(newSignature);

		ArchetypeBucket& oldBucket = ecs.archetypes[oldArchetypeIndex];
		ArchetypeBucket& newBucket = ecs.archetypes[newArchetypeIndex];
		const std::size_t newRow = newBucket.entities.size();

		newBucket.entities.push_back(entity);
		ReserveComponentSlotsForEntity(newBucket, newSignature);
		CopySharedComponents(oldBucket, oldRow, newBucket, newSignature);
		RemoveComponentFromBucket(oldBucket, componentType, oldRow);

		EraseRowFromArchetype(oldArchetypeIndex, oldRow);
		ecs.entityToArchetype[entity] = newArchetypeIndex;
		ecs.entityToArchetypeSlot[entity] = newRow;
		sigIt->second = newSignature;
	}

	std::vector<Entity> EntityManager::CollectEntitiesBySignature(const ComponentSignature& requiredSignature, bool activeOnly) const {
		std::vector<Entity> entities;
		for (const EntityStorage::ArchetypeBucket& bucket : ecs.archetypes) {
			if ((bucket.signature & requiredSignature) != requiredSignature) continue;

			for (Entity entity : bucket.entities) {
				auto propsIt = ecs.properties.find(entity);
				if (propsIt == ecs.properties.end()) continue;
				if (activeOnly && !propsIt->second.isActive) continue;
				entities.push_back(entity);
			}
		}
		return entities;
	}

	// ---------------------------------------------------------------------------
	// EntityManager APIs
	// ---------------------------------------------------------------------------
	Entity EntityManager::CreateEntity() {
		ComponentSignature emptySignature; // New entity starts with no components

		uint32_t entityIndex = 0u;
		if (!ecs.freeEntityIndices.empty()) {
			entityIndex = ecs.freeEntityIndices.back();
			ecs.freeEntityIndices.pop_back();
		}
		else {
			entityIndex = nextEntityID;
			if (entityIndex == 0u || entityIndex > ENTITY_INDEX_MASK) {
				throw std::runtime_error("CreateEntity: Entity index capacity exceeded.");
			}
			++nextEntityID;
		}

		if (ecs.entityGenerations.size() <= entityIndex) {
			ecs.entityGenerations.resize(static_cast<std::size_t>(entityIndex) + 1u, 1u);
		}

		uint32_t generation = ecs.entityGenerations[entityIndex];
		if (generation == 0u) {
			generation = 1u;
			ecs.entityGenerations[entityIndex] = generation;
		}

		Entity newEntity = MakeEntity(entityIndex, generation);
		ecs.entitySignatures[newEntity] = emptySignature;
		MoveEntityToArchetype(newEntity, emptySignature);

		std::string entityName = "GameObject (" + std::to_string(entityIndex) + ")";
		ecs.properties[newEntity] = EntityProperties{ entityName, CollisionLayer::Default };
		ecs.properties[newEntity].uuid = GenerateUUID();

		return newEntity;
	}

	Entity EntityManager::CreateEntityWithID(Entity entityID) {
		if (entityID == INVALID_ENTITY) {
			throw std::runtime_error("CreateEntityWithID: Invalid entity ID 0.");
		}

		uint32_t entityIndex = EntityToIndex(entityID);
		uint32_t generation = EntityToGeneration(entityID);

		// Backwards compatibility for legacy raw IDs with no generation bits.
		if (generation == 0u) generation = 1u;
		Entity normalizedEntity = MakeEntity(entityIndex, generation);

		ComponentSignature emptySignature; // New entity starts with no components
		ecs.entitySignatures[normalizedEntity] = emptySignature;
		MoveEntityToArchetype(normalizedEntity, emptySignature);

		std::string entityName = "GameObject (" + std::to_string(entityIndex) + ")";
		ecs.properties[normalizedEntity] = EntityProperties{ entityName, CollisionLayer::Default };
		ecs.properties[normalizedEntity].uuid = GenerateUUID();

		if (ecs.entityGenerations.size() <= entityIndex) {
			ecs.entityGenerations.resize(static_cast<std::size_t>(entityIndex) + 1u, 1u);
		}
		ecs.entityGenerations[entityIndex] = generation;

		ecs.freeEntityIndices.erase(
			std::remove(ecs.freeEntityIndices.begin(), ecs.freeEntityIndices.end(), entityIndex),
			ecs.freeEntityIndices.end()
		);

		if (entityIndex >= nextEntityID) {
			nextEntityID = entityIndex + 1u;
		}

		return normalizedEntity;
	}

	Entity EntityManager::CloneEntity(const Entity& entity) {
		if (ecs.entitySignatures.find(entity) == ecs.entitySignatures.end()) {
			throw std::runtime_error("Attempted to clone a non existent entity!");
		}

		Entity newEntity = CreateEntity();
		GetProperties(newEntity) = GetProperties(entity);
		GetProperties(newEntity).uuid = GenerateUUID();

		for (int i = 0; i < ComponentType::MAX_COMPONENTS; ++i) {
			ComponentType componentType = static_cast<ComponentType>(i);

			if (HasComponent(entity, componentType)) {
				switch (componentType) {
					case ComponentType::TransformType: AddComponent<Transform>(newEntity, GetComponent<Transform>(entity)); break;
					case ComponentType::VelocityType: AddComponent<Velocity>(newEntity, GetComponent<Velocity>(entity)); break;
					case ComponentType::AccelerationType: AddComponent<Acceleration>(newEntity, GetComponent<Acceleration>(entity)); break;
					case ComponentType::TriangleType: AddComponent<Triangle>(newEntity, GetComponent<Triangle>(entity)); break;
					case ComponentType::RigidBodyType: AddComponent<RigidBody>(newEntity, GetComponent<RigidBody>(entity)); break;
					case ComponentType::AABBColliderType: AddComponent<AABBCollider>(newEntity, GetComponent<AABBCollider>(entity)); break;
					case ComponentType::PlayerStateType: AddComponent<PlayerState>(newEntity, GetComponent<PlayerState>(entity)); break;
					case ComponentType::RotationFlagType: AddComponent<RotationFlag>(newEntity, GetComponent<RotationFlag>(entity)); break;
					case ComponentType::TrapPlatformComponentType: AddComponent<TrapPlatformComponent>(newEntity, GetComponent<TrapPlatformComponent>(entity)); break;
					case ComponentType::ButtonType: AddComponent<Button>(newEntity, GetComponent<Button>(entity)); break;
					case ComponentType::TextureComponentType: AddComponent<TextureComponent>(newEntity, GetComponent<TextureComponent>(entity)); break;
					case ComponentType::AudioComponentType: AddComponent<AudioComponent>(newEntity, GetComponent<AudioComponent>(entity)); break;
					case ComponentType::AnimationComponentType: AddComponent<AnimationComponent>(newEntity, GetComponent<AnimationComponent>(entity)); break;
					case ComponentType::TextComponentType: AddComponent<TextComponent>(newEntity, GetComponent<TextComponent>(entity)); break;
					case ComponentType::CameraType: AddComponent<Camera>(newEntity, GetComponent<Camera>(entity)); break;
					case ComponentType::Transform3DType: AddComponent<Transform3D>(newEntity, GetComponent<Transform3D>(entity)); break;
					case ComponentType::Primitive3DComponentType: AddComponent<Primitive3D>(newEntity, GetComponent<Primitive3D>(entity)); break;
					case ComponentType::DirectionalLight3DType: AddComponent<DirectionalLight3D>(newEntity, GetComponent<DirectionalLight3D>(entity)); break;
					case ComponentType::NativeScriptsComponentType: {
						NativeScriptsComponent copy = GetComponent<NativeScriptsComponent>(entity);
						for (NativeScriptsComponent::NativeScript& script : copy.scripts) {
							script.runtimeScriptId = NativeScriptsComponent::NativeScript::NextRuntimeScriptId();
						}
						AddComponent<NativeScriptsComponent>(newEntity, copy);
						break;
					}
					case ComponentType::SceneOwnedType: AddComponent<SceneOwned>(newEntity, GetComponent<SceneOwned>(entity)); break;
					case ComponentType::FSMComponentType: AddComponent<FSMComponent>(newEntity, GetComponent<FSMComponent>(entity)); break;
					case ComponentType::ParticleComponentType: AddComponent<ParticleComponent>(newEntity, GetComponent<ParticleComponent>(entity)); break;
					case ComponentType::ParticleEmitterComponentType: AddComponent<ParticleEmitterComponent>(newEntity, GetComponent<ParticleEmitterComponent>(entity)); break;
					case ComponentType::SliderType: AddComponent<Slider>(newEntity, GetComponent<Slider>(entity)); break;
					case ComponentType::PlayerHealthComponentType: AddComponent<PlayerHealthComponent>(newEntity, GetComponent<PlayerHealthComponent>(entity)); break;
					default:
						break;
				}
			}
		}

		return newEntity;
	}

	void EntityManager::DestroyEntity(Entity entity) {
		auto signatureIt = ecs.entitySignatures.find(entity);
		if (signatureIt == ecs.entitySignatures.end()) {
			return;
		}

		// True Destruction of an entity
		ComponentSignature signature = signatureIt->second;
		for (int i = 0; i < ComponentType::MAX_COMPONENTS; ++i) {
			ComponentType componentType = static_cast<ComponentType>(i);
			if (signature.test(componentType)) {
				RemoveComponent(entity, componentType);
			}
		}

		RemoveEntityFromArchetype(entity);
		ecs.entitySignatures.erase(signatureIt);
		ecs.properties.erase(entity);

		const uint32_t entityIndex = EntityToIndex(entity);
		if (entityIndex > 0u) {
			if (ecs.entityGenerations.size() <= entityIndex) {
				ecs.entityGenerations.resize(static_cast<std::size_t>(entityIndex) + 1u, 1u);
			}

			uint32_t& generation = ecs.entityGenerations[entityIndex];
			if (generation >= ENTITY_GENERATION_MASK) generation = 1u;
			else ++generation;

			ecs.freeEntityIndices.push_back(entityIndex);
		}

		Logger::Get().Info(DebugType::GameManager) << "Destroyed Entity with ID: " << entity << std::endl;
	}

	EntityProperties& EntityManager::GetProperties(Entity entity) { 
		if (ecs.properties.find(entity) != ecs.properties.end()) {
			return ecs.properties.at(entity);
		}
		else {
			throw GameException(DebugType::Editor, "Corrupted Object! Properties are invalid. Please use EntityManager's CreateEntity API to instantiate objects!");
		}
	}

	bool EntityManager::ValidateEntity(Entity entity) {
		if (ecs.properties.find(entity) == ecs.properties.end()) {
			return false;
		}
		
		return true;
	}

	uint64_t EntityManager::GenerateUUID() {
		static std::random_device rd;
		static std::mt19937 rng(rd());
		static std::uniform_int_distribution<uint64_t> dist;

		uint64_t gen = 0;
		while (gen == 0) gen = dist(rng);
		return gen;
	}

	bool EntityManager::HasComponent(Entity entity, ComponentType componentType) const {
		std::unordered_map<Entity, ComponentSignature>::iterator it = ecs.entitySignatures.find(entity);
		if (it == ecs.entitySignatures.end()) return false;
		return it->second.test(componentType);
	}

	void EntityManager::AddComponent(Entity entity, ComponentType componentType) {
		if (!ValidateEntity(entity)) {
			std::stringstream errorMsg;
			errorMsg << "AddComponent: Invalid Entity ID: " << entity;
			throw std::runtime_error(errorMsg.str());
		}

		if (HasComponent(entity, componentType)) return;

		switch (componentType)
		{
		case ComponentType::TransformType: { Transform c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::VelocityType: { Velocity c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::AccelerationType: { Acceleration c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::TriangleType: { Triangle c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::RigidBodyType: { RigidBody c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::AABBColliderType: { AABBCollider c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::PlayerStateType: { PlayerState c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::RotationFlagType: { RotationFlag c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::TrapPlatformComponentType: { TrapPlatformComponent c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::TextureComponentType: { TextureComponent c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::AudioComponentType: { AudioComponent c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::AnimationComponentType: { AnimationComponent c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::TextComponentType: { TextComponent c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::ButtonType: { Button c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::CameraType: { Camera c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::Transform3DType: { Transform3D c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::Primitive3DComponentType: { Primitive3D c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::DirectionalLight3DType: { DirectionalLight3D c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::NativeScriptsComponentType: { NativeScriptsComponent c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::FSMComponentType: { FSMComponent c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::ParticleComponentType: { ParticleComponent c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::ParticleEmitterComponentType: { ParticleEmitterComponent c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::SliderType: { Slider c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::PlayerHealthComponentType: { PlayerHealthComponent c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::PlayerTagType:
		case ComponentType::PlatformTagType:
		case ComponentType::BackgroundTagType:
		case ComponentType::EnemyTagType: ; break;
		case ComponentType::SceneOwnedType: { SceneOwned c{}; AddComponentInternal(entity, componentType, &c); break; }
		case ComponentType::PersistentTagType:
		case ComponentType::LoadedTagType: break;
		case ComponentType::GUITagType: break;
		case ComponentType::SpawnPointTagType: break;
		case ComponentType::ViewportConstrainedTagType: break;

		default:
			Logger::Get().Error(DebugType::GameManager) << "AddComponent: Unsupported ComponentType " << static_cast<int>(componentType) << std::endl;
			return;
		}

		if (IsTagComponent(componentType)) {
			AddComponentInternal(entity, componentType, nullptr);
		}
	}

	void EntityManager::RemoveComponent(Entity entity, ComponentType componentType) {
		RemoveComponentInternal(entity, componentType);
	}

	bool EntityManager::IsTagComponent(ComponentType componentType) {
		switch (componentType)
		{
		case ComponentType::PlayerTagType:
		case ComponentType::PlatformTagType:
		case ComponentType::BackgroundTagType:
		case ComponentType::EnemyTagType:
		case ComponentType::PersistentTagType:
		case ComponentType::LoadedTagType:
		case ComponentType::GUITagType:
		case ComponentType::SpawnPointTagType:
			return true;
case ComponentType::ViewportConstrainedTagType: return true;
		default:
			return false;
		}
	}

	void EntityManager::ClearTags(Entity entity) {
		for (int i = 0; i < ComponentType::MAX_COMPONENTS; ++i) {
			ComponentType componentType = static_cast<ComponentType>(i);
			if (IsTagComponent(componentType)) {
				RemoveComponent(entity, componentType);
			}
		}
	}

	bool EntityManager::IsActive(Entity entity) const {
		auto it = ecs.properties.find(entity);
		if (it != ecs.properties.end()) {
			return it->second.isActive;
		}
		else {
			Logger::Get().Info(DebugType::EntityManager) << "IsActive: Entity no longer exists!" << std::endl;
			return false;
		}
	}

	void EntityManager::SetActive(Entity entity, bool value) {
		auto it = ecs.properties.find(entity);
		if (it != ecs.properties.end()) {
			it->second.isActive = value;
		}
		else {
			Logger::Get().Info(DebugType::EntityManager) << "SetActive: Entity no longer exists!" << std::endl;
			return;
		}
	}

	std::size_t EntityManager::AttachScriptByName(Entity entity, std::string typeName, bool enabled) {
		if (!HasComponent(entity, NativeScriptsComponentType)) {
			AddComponent<NativeScriptsComponent>(entity, NativeScriptsComponent{});
		}

		NativeScriptsComponent& nativeScripts = GetComponent<NativeScriptsComponent>(entity);
		nativeScripts.scripts.emplace_back();
		NativeScriptsComponent::NativeScript& scriptToAdd = nativeScripts.scripts.back();
		scriptToAdd.enabled = enabled;

		if (!ScriptRegistry::Get().BindScriptByName(scriptToAdd, typeName)) {
			nativeScripts.scripts.pop_back();
			Logger::Get().Error(DebugType::GameManager)	<< "AttachScriptByName: Unknown Script '" << typeName << "'" << std::endl;
			return static_cast<std::size_t>(-1);
		}

		return nativeScripts.scripts.size() - 1;
	}

	std::size_t EntityManager::AttachCSharpScript(Entity entity, std::string typeName, std::string sourcePath, bool enabled) {
		if (!HasComponent(entity, NativeScriptsComponentType)) {
			AddComponent<NativeScriptsComponent>(entity, NativeScriptsComponent{});
		}

		NativeScriptsComponent& nativeScripts = GetComponent<NativeScriptsComponent>(entity);
		nativeScripts.scripts.emplace_back();
		NativeScriptsComponent::NativeScript& scriptToAdd = nativeScripts.scripts.back();
		scriptToAdd.enabled = enabled;

		if (typeName.empty()) {
			nativeScripts.scripts.pop_back();
			Logger::Get().Error(DebugType::GameManager) << "AttachCSharpScript: Script type cannot be empty." << std::endl;
			return static_cast<std::size_t>(-1);
		}

		scriptToAdd.BindCSharp(typeName, sourcePath);
		return nativeScripts.scripts.size() - 1;
	}

	void EntityManager::RemoveScriptAt(Entity entity, std::size_t index, bool removeComponentIfEmpty) {
		if (!HasComponent(entity, NativeScriptsComponentType)) return;

		NativeScriptsComponent& nativeScripts = GetComponent<NativeScriptsComponent>(entity);

		if (index >= nativeScripts.scripts.size()) {
			Logger::Get().Info(DebugType::GameManager) << "RemoveScriptAt: Provided index is greater than the number of scripts this entity has." << std::endl;
			return;
		}

		NativeScriptsComponent::NativeScript& targetScript = nativeScripts.scripts[index];

		CSharpScriptEngine::Get().DestroyScriptInstance(entity, targetScript);
		targetScript.Reset();

		nativeScripts.scripts.erase(nativeScripts.scripts.begin() + index);

		if (removeComponentIfEmpty && nativeScripts.scripts.empty()) RemoveComponent(entity, NativeScriptsComponentType);
	}

	void EntityManager::SwapScripts(Entity entity, std::size_t a, std::size_t b) {
		if (!HasComponent(entity, NativeScriptsComponentType)) {
			Logger::Get().Info(DebugType::GameManager) << "SwapScripts: Entity does not have any scripts!" << std::endl;
			return;
		}

		NativeScriptsComponent& nativeScripts = GetComponent<NativeScriptsComponent>(entity);

		if (a >= nativeScripts.scripts.size() || b >= nativeScripts.scripts.size() || a < 0 || b < 0) {
			Logger::Get().Info(DebugType::GameManager) << "SwapScripts: Invalid Index used for swapping!" << std::endl;
			return;
		}

		std::swap(nativeScripts.scripts[a], nativeScripts.scripts[b]);
	}

	std::vector<Entity> EntityManager::GetAllEntities() const {
		std::vector<Entity> entities;
		entities.reserve(ecs.properties.size());
		for (const auto& pair : ecs.properties) {
			entities.push_back(pair.first);
		}
		return entities;
	}

	std::vector<Entity> EntityManager::GetAllEntitiesWithComponent(bool activeOnly, ComponentType componentType) const {
		ComponentSignature requiredSignature;
		requiredSignature.set(static_cast<std::size_t>(componentType));
		return CollectEntitiesBySignature(requiredSignature, activeOnly);
	}

#pragma endregion

#pragma region WIP ENTITYMANAGER

	// ---------------------------------------------------------------------------
	// Constructors
	// ---------------------------------------------------------------------------
	WIPEntityManager::WIPEntityManager(WIPEntityStorage& entityStorage) : ecs(entityStorage) {};

	// ---------------------------------------------------------------------------
	// EntityManager APIs
	// ---------------------------------------------------------------------------
	Entity WIPEntityManager::CreateEntity() {
		using Archetype = WIPEntityStorage::Archetype;

		ComponentSignature emptySignature; // New entity starts with no components
		Entity newEntity = nextEntityID++;

		size_t archetypeIndex = ecs.archetypes.size();
		for (size_t i = 0; i < ecs.archetypes.size(); ++i) {
			// If an archetype with an empty signature exists, use it
			if (ecs.archetypes[i].signature.none()) {
				archetypeIndex = i;
				break;
			}
		}

		// If there is no empty archetype, create one
		if (archetypeIndex == ecs.archetypes.size()) {
			Archetype newArch;
			newArch.signature = emptySignature;
			ecs.archetypes.push_back(newArch);
		}

		Archetype& emptyArchetype = ecs.archetypes[archetypeIndex];
		emptyArchetype.entities.push_back(newEntity);

		ecs.entityToArchetype[newEntity] = archetypeIndex;
		ecs.entitySignatures[newEntity] = emptySignature;

		return newEntity;
	}

	Entity WIPEntityManager::CloneEntity(/*Entity& entity*/) {
		using Archetype = WIPEntityStorage::Archetype;

		ComponentSignature otherSignature;
		Entity newEntity = nextEntityID++;

		return newEntity;
	}

	bool WIPEntityManager::HasComponent(Entity entity, ComponentType componentType) const {
		auto it = ecs.entitySignatures.find(entity);
		if (it != ecs.entitySignatures.end()) {
			const ComponentSignature& signature = it->second;
			return signature.test(componentType);
		}

		return false; // Entity not found or does not have the component
	}

	Entity EntityManager::GetEntityByName(const std::string& name) const {
		// If exact match found, return immediately
		for (const auto& kv : ecs.properties) {
			const Entity entity = kv.first;
			const EntityProperties& props = kv.second;
			if (props.name == name) return entity;
		}

		Logger::Get().Info(DebugType::EntityManager) << "GetEntityByName: Entity with name '" << name << "' not found." << std::endl;
		return static_cast<Entity>(-1); // Return invalid entity
	}

	Entity EntityManager::GetEntityByUUID(uint64_t uuid) const {
		if (uuid == 0) return static_cast<Entity>(-1);
		for (const auto& kv : ecs.properties) {
			if (kv.second.uuid == uuid) return kv.first;
		}
		Logger::Get().Info(DebugType::EntityManager) << "GetEntityByUUID: Entity with UUID " << uuid << " not found." << std::endl;
		return static_cast<Entity>(-1);
	}

	std::vector<Entity> EntityManager::GetEntitiesBySubstring(const std::string& subStr) const {
		std::vector<Entity> matches;

		// Iterate through all entities
		for (const auto& kv : ecs.properties) {
			const Entity entity = kv.first;
			const EntityProperties& props = kv.second;

			if (props.name.find(subStr) != std::string::npos) {
				matches.push_back(entity);
			}
		}

		// Log if nothing was found (useful for debugging typos in names)
		if (matches.empty()) {
			Logger::Get().Info(DebugType::EntityManager)
				<< "GetEntitiesBySubstring: No entities found containing '" << subStr << "'." << std::endl;
		}

		return matches;
	}

#pragma endregion
}
