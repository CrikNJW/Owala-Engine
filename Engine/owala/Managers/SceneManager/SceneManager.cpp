/*****************************************************************//**
 * \file	SceneSystem.cpp
 * \brief	Manager responsible for managing scenes/levels
 *
 *
 * \author	Jason Lou
 * \date	11 September 2025
 *********************************************************************/
#include "SceneManager.h"
#include "EntityStorage.h"
#include "EntityManager.h"
#include "Logger.h"
#include "EntitySerializer.h"
#ifdef BUILD_EDITOR
#include "Editor.h"
#endif

namespace Framework {
	using namespace SceneManagement;

	// ---------------------------------------------------------------------------
	// Singleton Stuff
	// ---------------------------------------------------------------------------
	SceneManager* SceneManager::instance = nullptr; // Initialize the static instance pointer to nullptr
	SceneGUID SceneManager::nextGUID = 1; // Start from 1, 0 is reserved for invalid scene. This is set during compile time.
	const std::string INVALID_SCENENAME = "InvalidScene"; // Reserved name for invalid scene

	// Constructor
	SceneManager::SceneManager() {
		invalidScene.sceneIndex = 0;
		invalidScene.name = INVALID_SCENENAME;
		invalidScene.path = {};
		invalidScene.isLoaded = false;
	}


	SceneManager& SceneManager::GetInstance() {
		if (!instance) {
			instance = new SceneManager();
			Logger::Get().Info(DebugType::SceneManager) << "Scene System Instantiated!" << std::endl;
		}
		return *instance;
	}

	bool SceneManager::Exists() {
		return instance != nullptr;
	}

	void SceneManager::DestroyInstance() {
		delete instance;
		instance = nullptr;
	}

	// ---------------------------------------------------------------------------
	// SceneManager Checkers
	// ---------------------------------------------------------------------------
	void SceneManager::CheckStorage() {
		if (!entityStorageRef_) {
			throw GameException(DebugType::SceneManager, "Entity Storage must be set before use!");
		}
	}

	void SceneManager::CheckStorage() const {
		if (!entityStorageRef_) {
			throw GameException(DebugType::SceneManager, "Entity Storage must be set before use!");
		}
	}

	// ---------------------------------------------------------------------------
	// SceneManager Internals
	// ---------------------------------------------------------------------------

	SceneAsset* SceneManager::GetSceneAssetMutable(SceneGUID sceneIndex) {
		auto it = scenes_.find(sceneIndex);
		if (it != scenes_.end()) {
			return &(it->second);
		}

		return nullptr; // Return nullptr if scene not found
	}

	bool SceneManager::IsInPlayMode() const {
#ifdef BUILD_EDITOR
		return editorRef_ && editorRef_->IsPlaying();
#else
		return true; // Always in Play Mode during runtime
#endif
	}

	// ---------------------------------------------------------------------------
	// SceneManager Configuration
	// ---------------------------------------------------------------------------
	void SceneManager::SetEntityStorageRef(EntityStorage& entityStorageRef) {
		entityStorageRef_ = &entityStorageRef;
		Logger::Get().Info(DebugType::SceneManager) << "Successfully linked to Entity Storage" << std::endl;

	}

	// ---------------------------------------------------------------------------
	// Scene Management APIs
	// ---------------------------------------------------------------------------
	#pragma region Scene Asset Stuff
	SceneGUID SceneManager::CreateSceneAsset(const std::string& sceneName, const std::string& path, const bool entitiesPopulated) {
		CheckStorage();
		// Check if scene with the same name already exists
		if (CheckSceneNameExists(sceneName)) {
			Logger::Get().Error(DebugType::SceneManager) << "Scene with name '" << sceneName << "' already exists. Creation failed." << std::endl;
			return 0;
		}

		SceneAsset newScene;
		newScene.sceneIndex = nextGUID++;
		newScene.name = sceneName;
		newScene.path = path;
		newScene.isPopulated = entitiesPopulated;
		scenes_[newScene.sceneIndex] = newScene;
		Logger::Get().Info(DebugType::SceneManager) << "Created new Scene: " << sceneName << " with ID: " << newScene.sceneIndex << std::endl;
		return newScene.sceneIndex;
	}

	const SceneAsset& SceneManager::GetSceneByName(const std::string& sceneName, bool silenceError) const {
		for (const auto& [id, scene] : scenes_) {
			if (scene.name == sceneName) {
				return scene;
			}
		}

		if (!silenceError)
			Logger::Get().Error(DebugType::SceneManager) << "Scene with name '" << sceneName << "' does not exist." << std::endl;
		return invalidScene;
	}

	const SceneAsset& SceneManager::GetSceneAt(SceneGUID sceneIndex, bool silenceError) const {
		for (const auto& [id, scene] : scenes_) {
			if (scene.sceneIndex == sceneIndex) {
				return scene;
			}
		}

		if (!silenceError)
			Logger::Get().Error(DebugType::SceneManager) << "Scene with ID '" << sceneIndex << "' does not exist." << std::endl;
		return invalidScene;
	}

	const SceneAsset& SceneManager::GetActiveScene() const {
		return GetSceneAt(currentScene_);
	}

	void SceneManager::LoadScene(const std::string& sceneName, LoadSceneMode mode) {
		SceneAsset sceneToLoad = GetSceneByName(sceneName);
		if (!sceneToLoad.IsValid()) {
			Logger::Get().Error(DebugType::SceneManager) << "Scene with name '" << sceneName << "' does not exist. Aborting Load." << std::endl;
			return;
		}

		LoadScene(sceneToLoad, mode);
	}

	void SceneManager::LoadScene(const SceneGUID sceneIndex, LoadSceneMode mode) {
		SceneAsset sceneToLoad = GetSceneAt(sceneIndex);
		if (!sceneToLoad.IsValid()) {
			Logger::Get().Error(DebugType::SceneManager) << "Scene with ID '" << sceneIndex << "' does not exist. Aborting Load." << std::endl;
			return;
		}

		LoadScene(sceneToLoad, mode);
	}

	void SceneManager::LoadScene(const SceneAsset& scene, LoadSceneMode mode) {
		if (!scene.IsValid()) {
			Logger::Info(DebugType::SceneManager) << "Scene does not exist! Aborting Load." << std::endl;
			return;
		}

		CheckStorage();
		EntityManager entityMgr(*entityStorageRef_);

		const bool isInPlayMode = IsInPlayMode();

		// Scene is already loaded.
		if (scenes_.at(scene.sceneIndex).isLoaded) {
#ifdef BUILD_EDITOR
			if (!isInPlayMode) {
				if (mode == LoadSceneMode::Single) {
					std::vector<SceneGUID> scenesToUnload(editorLoadedScenes_.begin(), editorLoadedScenes_.end());
					for (SceneGUID sceneIndex : scenesToUnload) {
						if (sceneIndex != scene.sceneIndex) UnloadScene(GetSceneAt(sceneIndex));
					}
					editorLoadedScenes_.clear();
				}
				editorLoadedScenes_.insert(scene.sceneIndex);
			}
#endif
			if (isInPlayMode) {
				if (mode == LoadSceneMode::Single) {
					std::vector<SceneGUID> scenesToUnload(runtimeLoadedScenes_.begin(), runtimeLoadedScenes_.end());
					for (SceneGUID sceneIndex : scenesToUnload) {
						if (sceneIndex != scene.sceneIndex) UnloadScene(GetSceneAt(sceneIndex));
					}
					runtimeLoadedScenes_.clear();
				}
				runtimeLoadedScenes_.insert(scene.sceneIndex);
			}

			SetActiveScene(GetSceneAt(scene.sceneIndex));
			Logger::Get().Info(DebugType::SceneManager)	<< "Scene already loaded: " << scene.name << " (GUID " << scene.sceneIndex << ")." << std::endl;
			return;
		}

		// Scene has not been loaded.
#ifdef BUILD_EDITOR
		if (!isInPlayMode && mode == LoadSceneMode::Single && !editorLoadedScenes_.empty()) {
			std::vector<SceneGUID> scenesToUnload(editorLoadedScenes_.begin(), editorLoadedScenes_.end());
			for (SceneGUID sceneIndex : scenesToUnload) {
				if (sceneIndex != scene.sceneIndex) UnloadScene(GetSceneAt(sceneIndex));
			}
			editorLoadedScenes_.clear();
		}
#endif
		if (isInPlayMode) {
			if (mode == LoadSceneMode::Single && !runtimeLoadedScenes_.empty()) {
				std::vector<SceneGUID> scenesToUnload(runtimeLoadedScenes_.begin(), runtimeLoadedScenes_.end());
				for (SceneGUID sceneIndex : scenesToUnload) {
					if (sceneIndex != scene.sceneIndex) UnloadScene(GetSceneAt(sceneIndex));
				}
				runtimeLoadedScenes_.clear();
			}
		}

		scenes_.at(scene.sceneIndex).isLoaded = true;

		// Deserialize and load entities belonging to target scene
		std::vector<Entity> loadedEntities;
		std::string sceneName = scene.name;
		EntitySerializer::DeserializeSceneData(*entityStorageRef_, scene.path, sceneName, loadedEntities);

		for (Entity loadedEntity : loadedEntities) {
			AddEntityToScene(scene, loadedEntity);
		}

#ifdef BUILD_EDITOR
		editorLoadedScenes_.insert(scene.sceneIndex);
#endif // BUILD_EDITOR
		if (isInPlayMode) runtimeLoadedScenes_.insert(scene.sceneIndex);
		SetActiveScene(GetSceneAt(scene.sceneIndex));

		Logger::Get().Info(DebugType::SceneManager) << "Loaded Scene: " << scene.name << " with GUID: " << scene.sceneIndex << std::endl;
	}

	void SceneManager::UnloadScene(const std::string& sceneName) {
		SceneAsset targetScene = GetSceneByName(sceneName);
		if (!targetScene.IsValid()) {
			Logger::Get().Error(DebugType::SceneManager) << " Scene with name '" << sceneName << "' does not exist. Aborting Unload." << std::endl;
			return;
		}

		UnloadScene(targetScene);
	}

	void SceneManager::UnloadScene(const SceneGUID sceneIndex) {
		SceneAsset targetScene = GetSceneAt(sceneIndex);
		if (!targetScene.IsValid()) {
			Logger::Get().Error(DebugType::SceneManager) << " Scene with ID '" << sceneIndex << "' does not exist. Aborting Unload." << std::endl;
			return;
		}

		UnloadScene(targetScene);
	}

	void SceneManager::UnloadScene(const SceneAsset& scene) {
		if (!scene.IsValid()) {
			Logger::Info(DebugType::SceneManager) << "Scene does not exist! Aborting Unload." << std::endl;
			return;
		}

		if (!scenes_.at(scene.sceneIndex).isLoaded) {
			Logger::Info(DebugType::SceneManager) << "Scene is not loaded! Aborting Unload." << std::endl;
			return;
		}

		CheckStorage();
		EntityManager entityMgr(*entityStorageRef_);

		const bool isInPlayMode = IsInPlayMode();

		for (Entity entity : entityMgr.GetAllEntitiesWithComponent(SceneOwnedType)) {
			const SceneOwned& owningScene = entityMgr.GetComponent<SceneOwned>(entity);

			if (owningScene.sceneName != scene.name) continue;
			if (entityMgr.HasComponent(entity, LoadedPersistentTagType)) continue;

			entityMgr.DestroyEntity(entity);
		}

		scenes_.at(scene.sceneIndex).isLoaded = false;
#ifdef BUILD_EDITOR
		editorLoadedScenes_.erase(scene.sceneIndex);
#endif
		runtimeLoadedScenes_.erase(scene.sceneIndex);

		// If the current scene was unloaded.
		if (currentScene_ == scene.sceneIndex) {
			prevScene_ = currentScene_;

			SceneGUID nextSceneIndex = 0;

#ifdef BUILD_EDITOR
			if (!isInPlayMode) {
				if (!editorLoadedScenes_.empty()) nextSceneIndex = *editorLoadedScenes_.begin();
			}
			else
#endif
			{
				if (!runtimeLoadedScenes_.empty()) nextSceneIndex = *runtimeLoadedScenes_.begin();
			}
			if (nextSceneIndex != 0) {
				const SceneAsset& nextSceneToLoad = GetSceneAt(nextSceneIndex);
				SetActiveScene(nextSceneToLoad);
			}
			else {
				currentScene_ = 0; // No active scene.
			}
		}

		Logger::Get().Info(DebugType::SceneManager) << "Unloaded Scene: " << scene.name << " (GUID " << scene.sceneIndex << ")" << std::endl;
	}

	bool SceneManager::SetActiveScene(const SceneAsset& scene) {
		if (!scene.IsValid()) {
			Logger::Info(DebugType::SceneManager) << "SetActiveScene: Invalid scene. Aborting." << std::endl;
			return false;
		}

		const bool isInPlayMode = IsInPlayMode();

		// Check if target scene is already the currently active scene
		if (currentScene_ == scene.sceneIndex) return true;

		// Check if target scene is loaded.
		if (!scenes_.at(scene.sceneIndex).isLoaded) {
			Logger::Info(DebugType::SceneManager) << "SetActiveScene: Scene (" << scene.name << ") is not loaded. Call LoadScene first." << std::endl;
			return false;
		}

#ifdef BUILD_EDITOR
		if (!isInPlayMode) {
			if (!editorLoadedScenes_.count(scene.sceneIndex) != 0) {
				Logger::Info(DebugType::SceneManager) << "SetActiveScene: editor does not have scene (" << scene.name << ") loaded. Call LoadScene first." << std::endl;
				return false;
			}
		}
		else
#endif
		{
			if (!runtimeLoadedScenes_.count(scene.sceneIndex) != 0) {
				Logger::Info(DebugType::SceneManager) << "SetActiveScene: runtime does not have scene (" << scene.name << ") loaded. Call LoadScene first." << std::endl;
				return false;
			}
		}

		prevScene_ = currentScene_;
		currentScene_ = scene.sceneIndex;

		Logger::Get().Info(DebugType::SceneManager) << "Active Scene set to: " << scene.name << " (GUID " << scene.sceneIndex << ")" << std::endl;

		return true;
	}
	#pragma endregion

	#pragma region Entity Scene Stuff
	void SceneManager::AddEntityToScene(const std::string& sceneName, Entity entity) {
		SceneAsset targetScene = GetSceneByName(sceneName);
		if (!targetScene.IsValid()) {
			Logger::Get().Error(DebugType::SceneManager) << " Scene with name '" << sceneName << "' does not exist. Cannot add Entity[" << entity << "] to it." << std::endl;
			return;
		}

		AddEntityToScene(targetScene, entity);
	}

	void SceneManager::AddEntityToScene(const SceneGUID sceneIndex, Entity entity) {
		SceneAsset targetScene = GetSceneAt(sceneIndex);
		if (!targetScene.IsValid()) {
			Logger::Get().Error(DebugType::SceneManager) << "Scene with ID '" << sceneIndex << "' does not exist. Cannot add Entity[" << entity + "] to it." << std::endl;
			return;
		}

		AddEntityToScene(targetScene, entity);
	}

	void SceneManager::AddEntityToScene(const SceneAsset& scene, Entity entity) {
		SceneAsset targetScene = GetSceneAt(scene.sceneIndex);

		if (!targetScene.IsValid()) {
			Logger::Get().Error(DebugType::SceneManager) << "Scene does not exist. Cannot add Entity[" << entity << "] to it." << std::endl;
			return;
		}

		if (!targetScene.isLoaded) {
			Logger::Get().Error(DebugType::SceneManager) << "Scene " << targetScene.name << " (GUID " << scene.sceneIndex << ")" << "is not loaded. Cannot add Entity[" << entity << "] to it." << std::endl;
			return;
		}

		CheckStorage();
		EntityManager entityMgr(*entityStorageRef_);

		const bool isInPlayMode = IsInPlayMode();

		// If entity does not have SceneOwned component, add the component.
		if (!entityMgr.HasComponent(entity, ComponentType::SceneOwnedType)) {
			entityMgr.AddComponent(entity, ComponentType::SceneOwnedType);
		}

		// Move the entity to specified scene.
		entityMgr.GetComponent<SceneOwned>(entity).sceneName = targetScene.name;

		// If entity is persistent and is added to currently active scene during runtime. Give it the transient persistent tag.
		if (isInPlayMode && entityMgr.HasComponent(entity, PersistentTagType) && targetScene.sceneIndex == currentScene_) {
			entityMgr.AddComponent(entity, LoadedPersistentTagType);
		}

		Logger::Get().Info(DebugType::SceneManager) << "Added Entity[" << entity << "] to Scene [" << targetScene.name + "]" << std::endl;
	}

	void SceneManager::SetEntityAsPersistent(Entity entity, bool shouldPersist) {
		CheckStorage();

		EntityManager entityMgr(*entityStorageRef_);
		if (shouldPersist) {
			entityMgr.AddComponent(entity, PersistentTag{});
			if (entityMgr.GetComponent<SceneOwned>(entity).sceneName == GetActiveScene().name) {
				entityMgr.AddComponent(entity, LoadedPersistentTagType);
			}
		}
		else {
			entityMgr.RemoveComponent(entity, PersistentTagType);
			entityMgr.RemoveComponent(entity, LoadedPersistentTagType);
		}
	}

	std::vector<Entity> SceneManager::GetEntitiesInScene(const SceneAsset& scene) const {
		std::vector<Entity> activeEntities;
		const SceneAsset& targetScene = GetSceneAt(scene.sceneIndex);

		if (!targetScene.IsValid()) return activeEntities;

		CheckStorage();

		const bool isInPlayMode = IsInPlayMode();

#ifdef BUILD_EDITOR
		const bool loadedInEditor = (editorLoadedScenes_.count(targetScene.sceneIndex) != 0);
		if (!isInPlayMode && !loadedInEditor) return activeEntities;
#endif
		const bool loadedInRuntime = (runtimeLoadedScenes_.count(targetScene.sceneIndex) != 0);
		if (isInPlayMode && !loadedInRuntime) return activeEntities;

		EntityManager entityMgr(*entityStorageRef_);

		for (Entity entity : entityMgr.GetAllEntitiesWithComponent(SceneOwnedType)) {
			const SceneOwned& owningScene = entityMgr.GetComponent<SceneOwned>(entity);

			if (isInPlayMode && entityMgr.HasComponent(entity, LoadedPersistentTagType)) {
				activeEntities.push_back(entity);
				continue;
			}

			if (owningScene.sceneName != targetScene.name) continue;
			activeEntities.push_back(entity);
		}

		return activeEntities;
	}

	std::vector<Entity> SceneManager::GetEntitiesInSceneWithComponents(const SceneAsset& scene, std::initializer_list<ComponentType> componentTypes) const {
		std::vector<Entity> activeEntities;
		const SceneAsset& targetScene = GetSceneAt(scene.sceneIndex);

		if (!targetScene.IsValid()) return activeEntities;

		CheckStorage();

		const bool isInPlayMode = IsInPlayMode();

#ifdef BUILD_EDITOR
		const bool loadedInEditor = (editorLoadedScenes_.count(targetScene.sceneIndex) != 0);
		if (!isInPlayMode && !loadedInEditor) return activeEntities;
#endif
		const bool loadedInRuntime = (runtimeLoadedScenes_.count(targetScene.sceneIndex) != 0);
		if (isInPlayMode && !loadedInRuntime) return activeEntities;

		EntityManager entityMgr(*entityStorageRef_);

		for (Entity entity : entityMgr.GetAllEntitiesWithComponents(componentTypes)) {
			if (!entityMgr.HasComponent(entity, SceneOwnedType)) continue;
			const SceneOwned& owningScene = entityMgr.GetComponent<SceneOwned>(entity);

			if (isInPlayMode && entityMgr.HasComponent(entity, LoadedPersistentTagType)) {
				activeEntities.push_back(entity);
				continue;
			}

			if (owningScene.sceneName != targetScene.name) continue;
			activeEntities.push_back(entity);
		}

		return activeEntities;
	}

	const std::unordered_map<SceneGUID, SceneAsset>& SceneManager::GetAllSceneAssets() const {
		return scenes_;
	}

	bool SceneManager::CheckSceneNameExists(const std::string& sceneName) {
		std::cout << scenes_.size() << std::endl;
		for (const auto& [id, scene] : scenes_) {
			// Skip check if lengths differ
			if (scene.name.length() != sceneName.length()) continue;
			// Else, check if names are equal. Return true if equal, continue loop if not equal.
			bool isEqual = std::equal(scene.name.begin(), scene.name.end(), sceneName.begin(),
				[](char c1, char c2) {
					return std::tolower(static_cast<unsigned char>(c1)) ==
						std::tolower(static_cast<unsigned char>(c2));
				});

			if (isEqual) return true;
		}

		return false;
	}

	void SceneManager::UpdateScenePath(const SceneGUID& sceneIndex, const std::string& newPath) {
		for (auto& [id, scene] : scenes_) {
			if (scene.sceneIndex == sceneIndex) {
				scene.path = newPath;
			}
		}
	}


	#pragma endregion

	// ---------------------------------------------------------------------------
	// DEBUG FUNCTIONS
	// ---------------------------------------------------------------------------
	void SceneManager::PrintAllScenesInfo() const {
		CheckStorage();

		EntityManager entityMgr(*entityStorageRef_);
		std::unordered_map<std::string, std::vector<Entity>> scenes;
		for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(SceneOwnedType)) {
			SceneOwned sceneOwned = entityMgr.GetComponent<SceneOwned>(entity);
			scenes[sceneOwned.sceneName].push_back(entity);
		}

		Logger::Get().Info(DebugType::SceneManager) << "------- All Scenes Info ------" << std::endl;
		Logger::Get().Info(DebugType::SceneManager) << "Total Scenes: " << scenes.size() << std::endl;
		
		for (const auto& [sceneName, entities] : scenes) {
			size_t persistentCount = 0;

			// Count how many entities in this scene are persistent
			for (const Entity e : entities) {
				if (entityMgr.HasComponent(e, PersistentTagType)) {
					++persistentCount;
				}
			}

			Logger::Get().Info(DebugType::SceneManager) << "Scene: " << sceneName << " | Entities: " << entities.size() << ", " << persistentCount << " Persistent" << std::endl;
		}

		Logger::Get().Info(DebugType::SceneManager) << "------------------------------" << std::endl;

		// Will add more detailed info later if needed
	}

	void SceneManager::PrintCurrentSceneInfo() const {
		CheckStorage();
		std::vector<Entity> activeEntities = GetEntitiesInScene(GetActiveScene());

		EntityManager entityMgr(*entityStorageRef_);
		if (!GetSceneAt(currentScene_, true).IsValid()) {
			Logger::Get().Info(DebugType::SceneManager) << "No active scene currently loaded." << std::endl;
			return;
		}
		
		Logger::Get().Info(DebugType::SceneManager) << "----- Current Scene Info -----" << std::endl;
		Logger::Get().Info(DebugType::SceneManager) << "Loaded Scene: " + GetActiveScene().name << std::endl;
		Logger::Get().Info(DebugType::SceneManager) << "Active Entities Count: " + activeEntities.size() << std::endl;

		std::ostream& logStream = Logger::Get().Info(DebugType::SceneManager);
		logStream << "Active Entities IDs: ";
		for (const Entity& entity : activeEntities) {
			logStream << "Entity[" << entity << "]";
			if (entityMgr.HasComponent(entity, LoadedPersistentTagType)) {
				logStream << " (Persistent)";
			}
			logStream << ", ";
		}
		logStream << std::endl;
		
		Logger::Get().Info(DebugType::SceneManager) << "------------------------------" << std::endl;
	}
}