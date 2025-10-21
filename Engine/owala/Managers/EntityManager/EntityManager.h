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

namespace Framework {
	class ArchetypeManager;

	// THIS CLASS IS A UTILITY CLASS. Its not meant to store any data. Use it for API calls only.
	class EntityManager {
	private:
		//EntityStorage& ecs;
		ArchetypeManager& archetypeManager;
		std::unordered_map<Entity, EntityLocation>& entityLocations;
	public:
		/* Constructors */
		EntityManager(ArchetypeManager& archetypemanager, std::unordered_map<Entity, EntityLocation>& entityLocs);

		/* EntityManager APIs */
		// Creates a new entity and returns its ID.
		Entity CreateEntity();
		// Clones the specified entity, creating a new entity with the same components and data, and returns the new entity's ID.
		Entity CloneEntity(Entity& entity);

		// Checks if the specified entity has the specified component type attached to it. Returns true if the component is present, false otherwise.
		bool HasComponent(Entity entity, ComponentType componentType) const;
		// Adds a component of type T to the specified entity. The Component must be a valid component type defined in Components.h
		template <typename T>
		void AddComponent(Entity entity, const T& component) {
			constexpr ComponentType type = ComponentTypeTrait<T>::value;

			// Skip if entity already has it
			if (HasComponent(entity, type)) return;

			EntityLocation& oldLoc = entityLocations.at(entity);
			Archetype* oldArchetype = oldLoc.archetype;
			ComponentSignature newSig = oldArchetype->signature;
			newSig.set(type, true);

			Archetype* newArchetype = archetypeManager.FindOrCreateArchetype(newSig);
			size_t newIndex = newArchetype->entities.size();
			newArchetype->entities.push_back(entity);

			// Move existing components from old to new
			for (size_t i = 0; i < MAX_COMPONENTS; ++i) {
				if (!oldArchetype->signature.test(i)) continue;
				auto* oldArray = oldArchetype->componentArrays[i].get();
				auto* newArray = newArchetype->componentArrays[i].get();
				if (oldArray && newArray) {
					// Use virtual Remove/Move for type-erased copy
					oldArray->MoveTo(*newArray, oldLoc.index);
				}
			}

			// Add the new component
			auto& newCompArray = *static_cast<ComponentArray<T>*>(newArchetype->componentArrays[type].get());
			newCompArray.data.push_back(component);

			// Remove entity from old archetype
			oldArchetype->RemoveEntity(oldLoc.index, entityLocations);

			// Update entity location
			entityLocations[entity] = { newArchetype, newIndex };
		}

		// Adds a empty component of specified ComponentType ID to the entity.
		void AddComponent(Entity entity, ComponentType componentType);
		// Removes a component of type T from the specified entity. The Component must be a valid component type defined in Components.h
		void RemoveComponent(Entity entity, ComponentType componentType);
		// Gets the component of type T attached to the specified entity. Throws an error if the entity does not have the component.
		template <typename T>
		T& GetComponent(Entity entity) {
			if (!HasComponent(entity, ComponentTypeTrait<T>::value))
				throw std::runtime_error("Entity does not have the requested component");

			auto& array = GetComponentArrayForEntity<T>(entity, entityLocations);
			size_t index = entityLocations.at(entity).index;
			return array.data[index];
		}

		// Gets the ComponentArray<T> for the specified entity.
		template<typename T>
		ComponentArray<T>& GetComponentArrayForEntity(Entity entity,
			std::unordered_map<Entity, EntityLocation>& entityLocations)
		{
			EntityLocation& loc = entityLocations.at(entity);
			auto* array = static_cast<ComponentArray<T>*>(loc.archetype->componentArrays[ComponentTypeTrait<T>::value].get());
			return *array;
		}
		// Gets the ArchetypeManager associated with this EntityManager.
		ArchetypeManager& GetArchetypeManager();
		// Checks if given component is a Tag Component. Returns false if it is a regular component.
		bool IsTagComponent(ComponentType componentType);
		// Gets all entities that have been created. Note that this includes entities that have no components.
		std::vector<Entity> GetAllEntities() const;
		// Gets all entities that have the specified component type attached to them.
		std::vector<Entity> GetAllEntitiesWithComponent(ComponentType componentType) const;
	};
}