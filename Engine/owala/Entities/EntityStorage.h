/*****************************************************************//**
 * \file   EntityStorage.h
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
#pragma once
#include "Entity.h"
#include "Components.h"
#include "SceneComponents.h"
#include <bitset> //For a bitset to track which components an entity has
#include <unordered_map> //For a hash map to store entity-component pairs
#include <vector>
#include <array>

namespace Framework {
	// Forward Declaration
	class EntityManager;
	class WIPEntityManager;

	enum ComponentType {
		PlayerStatsType = 0,
		PositionType,
		VelocityType,
		AccelerationType,
		TriangleType,
		RigidBodyType,
		AABBColliderType,
		PlayerStateType,
		RotationFlagType,
		TrapPlatformComponentType,
		ScalingPlatformComponentType,
		TextureComponentType,
		AudioComponentType,
		AnimationComponentType,
		TextComponentType,
		CameraType,
		PlayerTagType,
		PlatformTagType,
		BackgroundTagType,
		EnemyTagType,
		SceneOwnedType,
		PersistentTagType,
		ActiveTagType,
		MAX_COMPONENTS
	};

#pragma region COMPONENT TYPE DEFINITIONS
	// --------------------------------------------------------------------------------
	//							/!\	/!\	READ THIS /!\ /!\
	//	         Whenever you create a new component or tag. Please add it in
	//             enum and to the list of ComponentTypeTraits below - Jason
	// --------------------------------------------------------------------------------

	// Generic template
	template <typename T>
	struct ComponentTypeTrait;

	// Specializations for each real component
	template <> struct ComponentTypeTrait<PlayerStats> { static constexpr ComponentType value = PlayerStatsType; };
	template <> struct ComponentTypeTrait<Position> { static constexpr ComponentType value = PositionType; };
	template <> struct ComponentTypeTrait<Velocity> { static constexpr ComponentType value = VelocityType; };
	template <> struct ComponentTypeTrait<Acceleration> { static constexpr ComponentType value = AccelerationType; };
	template <> struct ComponentTypeTrait<Triangle> { static constexpr ComponentType value = TriangleType; };
	template <> struct ComponentTypeTrait<RigidBody> { static constexpr ComponentType value = RigidBodyType; };
	template <> struct ComponentTypeTrait<AABBCollider> { static constexpr ComponentType value = AABBColliderType; };
	template <> struct ComponentTypeTrait<PlayerState> { static constexpr ComponentType value = PlayerStateType; };
	template <> struct ComponentTypeTrait<RotationFlag> { static constexpr ComponentType value = RotationFlagType; };
	template <> struct ComponentTypeTrait<TrapPlatformComponent> { static constexpr ComponentType value = TrapPlatformComponentType; };
	template <> struct ComponentTypeTrait<ScalingPlatformComponent> { static constexpr ComponentType value = ScalingPlatformComponentType; };
	template <> struct ComponentTypeTrait<TextureComponent> { static constexpr ComponentType value = TextureComponentType; };
	template <> struct ComponentTypeTrait<AudioComponent> { static constexpr ComponentType value = AudioComponentType; };
	template <> struct ComponentTypeTrait<AnimationComponent> { static constexpr ComponentType value = AnimationComponentType; };
	template <> struct ComponentTypeTrait<TextComponent> { static constexpr ComponentType value = TextComponentType; };
	template <> struct ComponentTypeTrait<Camera> { static constexpr ComponentType value = CameraType;};
	template <> struct ComponentTypeTrait<PlayerTag> { static constexpr ComponentType value = PlayerTagType; };
	template <> struct ComponentTypeTrait<PlatformTag> { static constexpr ComponentType value = PlatformTagType; };
	template <> struct ComponentTypeTrait<BackgroundTag> { static constexpr ComponentType value = BackgroundTagType; };
	template <> struct ComponentTypeTrait<EnemyTag> { static constexpr ComponentType value = EnemyTagType; };
	template <> struct ComponentTypeTrait<SceneOwned> { static constexpr ComponentType value = SceneOwnedType; };
	template <> struct ComponentTypeTrait<PersistentTag> { static constexpr ComponentType value = PersistentTagType; };
	template <> struct ComponentTypeTrait<ActiveTag> { static constexpr ComponentType value = ActiveTagType; };

#pragma endregion

	//class EntityStorage{
	//private:
	//	std::unordered_map<Entity, ComponentSignature> entitySignatures; // Track components of each entity
	//	friend class EntityManager; // Allow EntityManager to access private members
	//private:
	//	//Tags
	//	//std::unordered_map<Entity, PlayerTag>		playerTag;
	//	//std::unordered_map<Entity, PlatformTag>		platformTag;
	//	//std::unordered_map<Entity, BackgroundTag>	backgroundTag;
	//	//std::unordered_map<Entity, EnemyTag>		enemyTag;

	//	//Basic Components
	//	std::unordered_map<Entity, PlayerStats> playerStats;
	//	std::unordered_map<Entity, Position>     position;
	//	std::unordered_map<Entity, Velocity>     velocity;
	//	std::unordered_map<Entity, Acceleration> acceleration;
	//	std::unordered_map<Entity, Triangle>     triangle;
	//	std::unordered_map<Entity, RigidBody>    rigidBody;    
	//	std::unordered_map<Entity, AABBCollider> colliders; 
	//	std::unordered_map<Entity, PlayerState> playerState;
	//	std::unordered_map<Entity, RotationFlag> rotationFlag;
	//	std::unordered_map<Entity, TrapPlatformComponent> trapPlatform;
	//	std::unordered_map<Entity, ScalingPlatformComponent> scalingPlatform;

	//	std::unordered_map<Entity, TextureComponent>      tex; //Texture Component
	//	std::unordered_map<Entity, AudioComponent>        audio; //Audio Component
	//	std::unordered_map<Entity, AnimationComponent> animatedTex; //Animated Texture Component
	//	std::unordered_map<Entity, TextComponent>	text; // Text Component

	//	std::unordered_map<Entity, Camera> cameras;//Camera

	//	// Scene Components
	//	std::unordered_map<Entity, SceneOwned>		sceneOwned; // Tag to indicate which scene this entity belongs to
	//	//std::unordered_map<Entity, PersistentTag>	persistentTag; // Tag to indicate if the entity should persist across scene changes
	//	//std::unordered_map<Entity, ActiveTag>		activeTag; // Tag to indicate if entity is to be loaded in the scene
	//};

	using ComponentSignature = std::bitset<MAX_COMPONENTS>; // Bitset to track which components an entity has
	struct Archetype;

	struct IComponentArray {
		virtual ~IComponentArray() = default;
		virtual void Remove(size_t index) = 0;

		// NEW: move a value to another component array
		virtual void MoveTo(IComponentArray& targetBase, size_t index) = 0;
	};

	template<typename T>
	struct ComponentArray : IComponentArray {
		std::vector<T> data;

		void Remove(size_t index) override {
			data[index] = std::move(data.back());
			data.pop_back();
		}

		void MoveTo(IComponentArray& targetBase, size_t index) override {
			auto& target = static_cast<ComponentArray<T>&>(targetBase);
			target.data.push_back(std::move(data[index]));
		}
	};

	struct EntityLocation {
		Archetype* archetype;
		size_t index;
	};

	struct Archetype {
		ComponentSignature signature;
		std::vector<Entity> entities;
		std::array<std::unique_ptr<IComponentArray>, MAX_COMPONENTS> componentArrays;

		// Removes an entity at the specified index from this archetype.
		void RemoveEntity(size_t index, std::unordered_map<Entity, EntityLocation>& entityLocations) {
			size_t last = entities.size() - 1;
			Entity lastEntity = entities.back();

			entities[index] = lastEntity;
			entities.pop_back();

			for (size_t i = 0; i < MAX_COMPONENTS; ++i) {
				if (signature.test(i) && componentArrays[i])
					componentArrays[i]->Remove(index);
			}

			if (index != last) {
				entityLocations[lastEntity].index = index;
			}
		}

		//Gets the ComponentArray<T> for this archetype, which holds components of type T.
		template<typename T>
		ComponentArray<T>* GetComponentArray() {
			constexpr ComponentType type = ComponentTypeTrait<T>::value;
			return static_cast<ComponentArray<T>*>(componentArrays[type].get());
		}
	};


	class ArchetypeManager {
	public:
		std::unordered_map<ComponentSignature, std::unique_ptr<Archetype>> archetypes;
		std::unordered_map<Entity, EntityLocation> entityLocations;

		Archetype* FindOrCreateArchetype(const ComponentSignature& sig) {
			auto it = archetypes.find(sig);
			if (it != archetypes.end())
				return it->second.get();

			auto archetype = std::make_unique<Archetype>();
			archetype->signature = sig;

			// Allocate component arrays for all components in this signature
			for (size_t i = 0; i < MAX_COMPONENTS; ++i) {
				if (sig.test(i))
					archetype->componentArrays[i] = CreateComponentArrayForIndex(i);
			}

			Archetype* ptr = archetype.get();
			archetypes[sig] = std::move(archetype);
			return ptr;
		}

		template<typename... Components>
		void ForEachArchetypeWith(auto&& callback) {
			for (auto& [sig, archPtr] : archetypes) {
				if ((sig.test(ComponentTypeTrait<Components>::value) && ...)) {
					callback(archPtr.get());
				}
			}
		}

	private:
		std::unique_ptr<IComponentArray> CreateComponentArrayForIndex(size_t index) {
			switch (index) {
			case PlayerStatsType: return std::make_unique<ComponentArray<PlayerStats>>();
			case PositionType: return std::make_unique<ComponentArray<Position>>();
			case VelocityType: return std::make_unique<ComponentArray<Velocity>>();
			case AccelerationType: return std::make_unique<ComponentArray<Acceleration>>();
			case TriangleType: return std::make_unique<ComponentArray<Triangle>>();
			case RigidBodyType: return std::make_unique<ComponentArray<RigidBody>>();
			case AABBColliderType: return std::make_unique<ComponentArray<AABBCollider>>();
			case PlayerStateType: return std::make_unique<ComponentArray<PlayerState>>();
			case RotationFlagType: return std::make_unique<ComponentArray<RotationFlag>>();
			case TrapPlatformComponentType: return std::make_unique<ComponentArray<TrapPlatformComponent>>();
			case ScalingPlatformComponentType: return std::make_unique<ComponentArray<ScalingPlatformComponent>>();
			case TextureComponentType: return std::make_unique<ComponentArray<TextureComponent>>();
			case AudioComponentType: return std::make_unique<ComponentArray<AudioComponent>>();
			case AnimationComponentType: return std::make_unique<ComponentArray<AnimationComponent>>();
			case TextComponentType: return std::make_unique<ComponentArray<TextComponent>>();
			case CameraType: return std::make_unique<ComponentArray<Camera>>();
			case PlayerTagType: return std::make_unique<ComponentArray<PlayerTag>>();
			case PlatformTagType: return std::make_unique<ComponentArray<PlatformTag>>();
			case BackgroundTagType: return std::make_unique<ComponentArray<BackgroundTag>>();
			case EnemyTagType: return std::make_unique<ComponentArray<EnemyTag>>();
			case SceneOwnedType: return std::make_unique<ComponentArray<SceneOwned>>();
			case PersistentTagType: return std::make_unique<ComponentArray<PersistentTag>>();
			case ActiveTagType: return std::make_unique<ComponentArray<ActiveTag>>();
			default: return nullptr;
			}
		}
	};

}