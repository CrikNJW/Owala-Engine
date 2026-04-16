/*****************************************************************//**
* \file   EntityManager.h
*
* \primary_author
*      Jason Lou
* \secondary_author
*
*
* \brief	Manager responsible for managing entities and their
*			components in the ECS architecture. Also serves as a
*			Entity Factory/Game Object Factory to create entities.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "EntityStorage.h"
#include <stdexcept>
#include <bitset>
#include <sstream>
#include <type_traits>

namespace Framework {

	// THIS CLASS IS A UTILITY CLASS. Its not meant to store any data. Use it for API calls only.
	class EntityManager {
	private:
		EntityStorage& ecs;
		using ArchetypeBucket = EntityStorage::ArchetypeBucket;

		std::size_t GetOrCreateArchetype(const ComponentSignature& signature);
		void EraseRowFromArchetype(std::size_t archetypeIndex, std::size_t rowIndex);
		void CopySharedComponents(const ArchetypeBucket& source, std::size_t sourceRow, ArchetypeBucket& target, const ComponentSignature& sharedSignature) const;
		void AddComponentToBucket(ArchetypeBucket& bucket, ComponentType componentType, const void* componentData);
		void RemoveComponentFromBucket(ArchetypeBucket& bucket, ComponentType componentType, std::size_t rowIndex);
		void ReserveComponentSlotsForEntity(ArchetypeBucket& bucket, const ComponentSignature& signature);
		void AddComponentInternal(Entity entity, ComponentType componentType, const void* componentData);
		void RemoveComponentInternal(Entity entity, ComponentType componentType);

		void RemoveEntityFromArchetype(Entity entity);
		void MoveEntityToArchetype(Entity entity, const ComponentSignature& signature);
		std::vector<Entity> CollectEntitiesBySignature(const ComponentSignature& requiredSignature, bool activeOnly) const;
	public:
		/* Constructors */
		EntityManager(EntityStorage& entityStorage);

		/* EntityManager APIs */
		// Creates a new entity and returns its ID.
		Entity CreateEntity();
		// Creates a new entity with the specified ID.
		// Use with caution, this will overwrite any existing entity with conflicting ID.
		Entity CreateEntityWithID(Entity entityID);
		// Clones the specified entity, creating a new entity with the same components and data, and returns the new entity's ID.
		Entity CloneEntity(const Entity& entity);
		// Destroys the specified entity, the entity and all its components are destroyed together.
		void DestroyEntity(Entity entity);

		// Gets the property data of the specified entity.
		EntityProperties& GetProperties(Entity entity);

		// Validate if Entity exists. Returns true if Entity is a valid entity.
		bool ValidateEntity(Entity entity);

		// Generates a UUID
		static uint64_t GenerateUUID();

		// Checks if the specified entity has the specified component type attached to it. Returns true if the component is present, false otherwise.
		bool HasComponent(Entity entity, ComponentType componentType) const;
		template <typename T>
		bool HasComponent(Entity entity) const {
			return HasComponent(entity, ComponentTypeTrait<T>::value);
		}
		// Adds a component of type T to the specified entity. The Component must be a valid component type defined in Components.h
		template <typename T>
		void AddComponent(Entity entity, const T& component) {
			if (!ValidateEntity(entity)) {
				std::stringstream errorMsg;
				errorMsg << "AddComponent: Invalid Entity ID: " << entity;
				throw std::runtime_error(errorMsg.str());
			}

			constexpr ComponentType type = ComponentTypeTrait<T>::value;

			if (HasComponent(entity, type)) return;
			AddComponentInternal(entity, type, &component);
		}
		// Adds a empty component of specified ComponentType ID to the entity.
		void AddComponent(Entity entity, ComponentType componentType);
		// Removes a component of type T from the specified entity. The Component must be a valid component type defined in Components.h
		void RemoveComponent(Entity entity, ComponentType componentType);
		template <typename T>
		void RemoveComponent(Entity entity) {
			RemoveComponent(entity, ComponentTypeTrait<T>::value);
		}
		// Gets the component of type T attached to the specified entity. Throws an error if the entity does not have the component.
		template <typename T>
		T& GetComponent(Entity entity) {
			// Get the component type from the trait
			constexpr ComponentType type = ComponentTypeTrait<T>::value;

			// Validate Entity
			if (!ValidateEntity(entity)) {
				std::stringstream errorMsg;
				errorMsg << "GetComponent: Invalid Entity ID: " << entity << " requested component with ID: " << static_cast<int>(type);
				throw std::runtime_error(errorMsg.str());
			}

			// Ensure the entity actually has this component
			if (!HasComponent(entity, type)) {
				std::stringstream errorMsg;
				errorMsg << "GetComponent: Entity: " << GetProperties(entity).name << " does not have the requested component with ID: " << static_cast<int>(type);
				throw std::runtime_error(errorMsg.str());
			}

			const std::size_t archetypeIndex = ecs.entityToArchetype.at(entity);
			const std::size_t rowIndex = ecs.entityToArchetypeSlot.at(entity);
			ArchetypeBucket& archetype = ecs.archetypes[archetypeIndex];

			if constexpr (std::is_same_v<T, Transform>) return archetype.transform[rowIndex];
			else if constexpr (std::is_same_v<T, Velocity>) return archetype.velocity[rowIndex];
			else if constexpr (std::is_same_v<T, Acceleration>) return archetype.acceleration[rowIndex];
			else if constexpr (std::is_same_v<T, Triangle>) return archetype.triangle[rowIndex];
			else if constexpr (std::is_same_v<T, RigidBody>) return archetype.rigidBody[rowIndex];
			else if constexpr (std::is_same_v<T, AABBCollider>) return archetype.colliders[rowIndex];
			else if constexpr (std::is_same_v<T, PlayerState>) return archetype.playerState[rowIndex];
			else if constexpr (std::is_same_v<T, RotationFlag>) return archetype.rotationFlag[rowIndex];
			else if constexpr (std::is_same_v<T, TrapPlatformComponent>) return archetype.trapPlatform[rowIndex];
			else if constexpr (std::is_same_v<T, TextureComponent>) return archetype.tex[rowIndex];
			else if constexpr (std::is_same_v<T, AudioComponent>) return archetype.audio[rowIndex];
			else if constexpr (std::is_same_v<T, AnimationComponent>) return archetype.animatedTex[rowIndex];
			else if constexpr (std::is_same_v<T, TextComponent>) return archetype.text[rowIndex];
			else if constexpr (std::is_same_v<T, Button>) return archetype.button[rowIndex];
			else if constexpr (std::is_same_v<T, SceneOwned>) return archetype.sceneOwned[rowIndex];
			else if constexpr (std::is_same_v<T, Camera>) return archetype.cameras[rowIndex];
			else if constexpr (std::is_same_v<T, NativeScriptsComponent>) return archetype.nativeScripts[rowIndex];
			else if constexpr (std::is_same_v<T, FSMComponent>) return archetype.fsmComponent[rowIndex];
			else if constexpr (std::is_same_v<T, ParticleComponent>) return archetype.particleComponent[rowIndex];
			else if constexpr (std::is_same_v<T, ParticleEmitterComponent>) return archetype.particleEmitterComponent[rowIndex];
			else if constexpr (std::is_same_v<T, Slider>) return archetype.sliders[rowIndex];
			else if constexpr (std::is_same_v<T, PlayerHealthComponent>) return archetype.playerHealth[rowIndex];
			else static_assert(!std::is_same_v<T, T>, "Unsupported component type");
		}
		template<typename T>
		T* TryGetComponent(Entity entity) {
			if (!ValidateEntity(entity)) return nullptr;
			if (!HasComponent<T>(entity)) return nullptr;
			return &GetComponent<T>(entity);
		}
		// Checks if given component is a Tag Component. Returns false if it is a regular component.
		bool IsTagComponent(ComponentType componentType);
		// Clears all tag components from the specified entity.
		void ClearTags(Entity entity);
		// The active state of the entity. True if active, false if inactive. (Read Only)
		bool IsActive(Entity entity) const;
		// Activates or deactivates the entity to be used by systems and scripts. 
		void SetActive(Entity entity, bool value);
		// Attaches a custom C++ script to an entity.
		template<class T>
		std::size_t AttachScript(Entity entity, const char* displayName = nullptr, bool enabled = true);
		// Attaches a custom C++ script to an entity.
		std::size_t AttachScriptByName(Entity entity, std::string typeName, bool enabled = true);
		// Removes a custom C++ script at the provided script index from an entity.
		void RemoveScriptAt(Entity entity, std::size_t index, bool removeComponentIfEmpty = true);
		// Swaps the index of an existing script with another bounded script.
		void SwapScripts(Entity entity, std::size_t a, std::size_t b);
		// Gets the script of type T attached to the specified entity. Returns a null pointer if the entity does not have the script.
		template<class T>
		T* GetScript(Entity entity);
		// Gets all entities that have been created. Note that this includes entities that have no components.
		std::vector<Entity> GetAllEntities() const;
		// Gets all entities that have the specified component type attached to them.
		std::vector<Entity> GetAllEntitiesWithComponent(bool activeOnly, ComponentType componentType) const;
		// Gets all entities that have all the specified component types attached to them. (Multiple component support)
		template<typename... Args>
		std::vector<Entity> GetAllEntitiesWithComponents(bool activeOnly, Args... componentTypes) const;
		template<typename Func, typename... Types>
		void ForEachEntityWithComponents(Func&& fn, bool activeOnly, Types... componentTypes);
		template<typename... Types>
		std::vector<Entity> QueryEntities(bool activeOnly = true) const {
			return GetAllEntitiesWithComponents(activeOnly, ComponentTypeTrait<Types>::value...);
		}
		// Get entity signatures
		inline const auto& GetEntitySignatures() const {
			return ecs.entitySignatures;
		}

		//Get entity by name
		Entity GetEntityByName(const std::string& name) const;
		std::vector<Entity> GetEntitiesBySubstring(const std::string& subStr) const;
		Entity GetEntityByUUID(uint64_t uuid) const;

	};

	// BELOW IS STILL WIP. DO NOT USE.
	class WIPEntityManager {
	private:
		WIPEntityStorage& ecs;
	public:
		/* Constructors */
		WIPEntityManager(WIPEntityStorage& entityStorage);

		/* EntityManager APIs */
		// Creates a new entity and returns its ID.
		Entity CreateEntity();
		// Clones the specified entity, creating a new entity with the same components and data, and returns the new entity's ID.
		Entity CloneEntity(/*Entity& entity*/);

		// Checks if the specified entity has the specified component type attached to it. Returns true if the component is present, false otherwise.
		bool HasComponent(Entity entity, ComponentType componentType) const;
		// Adds a component of type T to the specified entity. The Component must be a valid component type defined in Components.h
		//template <typename T>
		//void AddComponent(Entity entity, const T& component);
		// Adds a component of the specified ComponentType to the specified entity.
		//void AddComponent(Entity entity, ComponentType componentType);
		// Removes a component of type T from the specified entity. The Component must be a valid component type defined in Components.h
		//void RemoveComponent(Entity entity, ComponentType componentType);
	};

	template<class T>
	std::size_t EntityManager::AttachScript(Entity entity, const char* displayName, bool enabled)
	{
		if (!HasComponent(entity, NativeScriptsComponentType)) {
			AddComponent<NativeScriptsComponent>(entity, NativeScriptsComponent{});
		}

		NativeScriptsComponent& nativeScripts = GetComponent<NativeScriptsComponent>(entity);

		nativeScripts.scripts.emplace_back();
		NativeScriptsComponent::NativeScript& scriptToAdd = nativeScripts.scripts.back();
		scriptToAdd.enabled = enabled;
		scriptToAdd.Bind<T>(displayName ? displayName : typeid(T).name());

		return nativeScripts.scripts.size() - 1;
	}

	template<class T>
	T* EntityManager::GetScript(Entity entity) {
		static_assert(std::is_base_of<ScriptableEntity, T>::value, "T must derive from ScriptableEntity");

		if (!ValidateEntity(entity)) return nullptr;
		if (!HasComponent(entity, NativeScriptsComponentType)) return nullptr;

		NativeScriptsComponent& nativeScripts = GetComponent<NativeScriptsComponent>(entity);
		for (NativeScriptsComponent::NativeScript& script : nativeScripts.scripts) {
			if (!script.Instance) continue;
			if (!script.enabled) continue;
			if (script.missing) continue;

			if (T* casted = dynamic_cast<T*>(script.Instance)) {
				return casted;
			}
		}
		return nullptr;
	}

	template<typename... Args>
	std::vector<Entity> EntityManager::GetAllEntitiesWithComponents(bool activeOnly, Args... componentTypes) const {
		// Create the required signature bitset
		ComponentSignature requiredSignature;

		// This sets a bit for each component type passed in
		(requiredSignature.set(static_cast<size_t>(componentTypes)), ...);

		return CollectEntitiesBySignature(requiredSignature, activeOnly);
	}

	template <typename Func, typename... Types>
	void EntityManager::ForEachEntityWithComponents(Func&& fn, bool activeOnly, Types... componentTypes) {
		// Create the required signature bitset
		ComponentSignature requiredSignature;

		// This sets a bit for each component type passed in
		(requiredSignature.set(static_cast<size_t>(componentTypes)), ...);

		for (Entity entity : CollectEntitiesBySignature(requiredSignature, activeOnly)) {
			fn(entity);
		}
	}
}
