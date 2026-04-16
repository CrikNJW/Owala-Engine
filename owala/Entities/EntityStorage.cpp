/*****************************************************************//**
 * \file   EntityStorage.cpp
 *
 * \primary_author
 *      Jason Lou(60%)
 * \secondary_author
 *		Ng Jing Wen(40%)
 *
 * \brief	Contains the definition of EntityStorage class which stores
 *			unordered maps of entity-component pairs for all components.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "pch.h"
#include "EntityStorage.h"
#include "Components.h"

namespace Framework {

	/**
	 * Copy Constructor - Performs a deep copy of all components, including properly 
	 * handling the NativeScriptsComponent to ensure that script instances are not shared between EntityStorage copies.
	 * 
	 * \param other
	 */
	EntityStorage::EntityStorage(const EntityStorage& other) {
		entitySignatures = other.entitySignatures;
		archetypes = other.archetypes;
		signatureToArchetype = other.signatureToArchetype;
		entityToArchetype = other.entityToArchetype;
		entityToArchetypeSlot = other.entityToArchetypeSlot;
		entityGenerations = other.entityGenerations;
		freeEntityIndices = other.freeEntityIndices;
		properties = other.properties;
	}

	/**
	 * Copy Assignment Operator - Performs a deep copy of all components, including properly
	 * handling the NativeScriptsComponent to ensure that script instances are not shared between EntityStorage copies.
	 * 
	 * \param other
	 * \return 
	 */
	EntityStorage& EntityStorage::operator=(const EntityStorage& other) {
		if (this == &other) return *this;

		// Reset all existing NativeScript instances before copying new data
		for (ArchetypeBucket& archetype : archetypes) {
			for (NativeScriptsComponent& scripts : archetype.nativeScripts) {
				for (NativeScriptsComponent::NativeScript& script : scripts.scripts) {
					script.Reset();
				}
			}
		}

		entitySignatures = other.entitySignatures;
		archetypes = other.archetypes;
		signatureToArchetype = other.signatureToArchetype;
		entityToArchetype = other.entityToArchetype;
		entityToArchetypeSlot = other.entityToArchetypeSlot;
		entityGenerations = other.entityGenerations;
		freeEntityIndices = other.freeEntityIndices;
		properties = other.properties;
		return *this;
	}

	EntityStorage::~EntityStorage() {
		for (ArchetypeBucket& archetype : archetypes) {
			for (NativeScriptsComponent& scripts : archetype.nativeScripts) {
				for (NativeScriptsComponent::NativeScript& script : scripts.scripts) {
					script.Reset();
				}
			}
		}
	}

	/**
	 * Swaps the contents of this EntityStorage with another. 
	 * This is a noexcept operation that efficiently exchanges all component data and 
	 * metadata between the two storages without copying, including properly handling the 
	 * NativeScriptsComponent to ensure that script instances are not shared after the swap.
	 * 
	 * \param other
	 * \return 
	 */
	void EntityStorage::SwapWith(EntityStorage& other) noexcept {
		// Metadata
		std::swap(entitySignatures, other.entitySignatures);
		std::swap(archetypes, other.archetypes);
		std::swap(signatureToArchetype, other.signatureToArchetype);
		std::swap(entityToArchetype, other.entityToArchetype);
		std::swap(entityToArchetypeSlot, other.entityToArchetypeSlot);
		std::swap(entityGenerations, other.entityGenerations);
		std::swap(freeEntityIndices, other.freeEntityIndices);
		std::swap(properties, other.properties);
	}
}
