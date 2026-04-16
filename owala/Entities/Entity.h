/*****************************************************************//**
 * \file	Entity.h
 *
 * \primary_author
 *      Ng Jing Wen
 * \secondary_author

 *
 * \brief	Contains declaratiosn to create an entity and an entityID variable.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once //Ensure the file is only included once during compilation
#include <cstdint>
using Entity = uint32_t;

constexpr uint32_t ENTITY_INDEX_BITS = 20;
constexpr uint32_t ENTITY_INDEX_MASK = (1u << ENTITY_INDEX_BITS) - 1u;
constexpr uint32_t ENTITY_GENERATION_MASK = (1u << (32u - ENTITY_INDEX_BITS)) - 1u;
constexpr Entity INVALID_ENTITY = 0u;

inline uint32_t EntityToIndex(Entity entity) {
	return entity & ENTITY_INDEX_MASK;
}

inline uint32_t EntityToGeneration(Entity entity) {
	return (entity >> ENTITY_INDEX_BITS) & ENTITY_GENERATION_MASK;
}

inline Entity MakeEntity(uint32_t index, uint32_t generation) {
	return ((generation & ENTITY_GENERATION_MASK) << ENTITY_INDEX_BITS) | (index & ENTITY_INDEX_MASK);
}

extern Entity nextEntityID; // Global var to track next available entity ID

/**
 * \brief Returns a new entity ID and increments the global nextEntityID variable.
 * \return Entity - A new unique entity ID.
 */
Entity createEntity();
