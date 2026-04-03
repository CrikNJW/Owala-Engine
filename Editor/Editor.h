/*********************************************************************//**
* \file   Editor.h
*
* \primary_author
*		Jason Lou (16.66%)
* \secondary_author
*		Lur Pei Hao Stanley (16.66%)
*		Sitara Anil (16.66%)
*		Ng Jordain Ying Zhe (16.66%)
*		Yar Xuan (16.66%)
*		Ng Jing Wen (16.66%)
*
*
* \brief	Declares the Editor system for in-game level editing and
			debugging using ImGUI.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/

#pragma once
#include "ISystem.h"
#include <vector>
#include <string>
#include <filesystem>
#include "EntityStorage.h"
#include "PrefabSystem.h" // prefab header for creation
#include "Graphics.h"
#include "ResourceManager.h"
#include "SceneManager.h"
#include "PhysicsSystem.h"
#include "TransformationSystem.h"
#include "LayerManager.h"

// Editor Modules
#include "History.h"
#include "Gizmo.h"
#include "ProjectScaffold.h"
#include "BuildSizeAnalyzer.h"

// Imgui Dependencies
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_stdlib.h"
#include "AssetBrowser.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace Framework {
	class GameManager; // Forward Declaration
	class CoreEngine; // Forward Declaration
	class CameraSystem;
	struct SystemTime; // Forward Declaration
}

namespace Editor {
	// Helper function to convert component type to string
	std::string ComponentToString(Framework::ComponentType componentType);


	/*********************************************************************//**
	 * \brief   Configuration structure defining the visual layout and active
	 *          windows for the Editor interface.
	 *********************************************************************/
	struct EditorSettings {
		int windowWidth = 1920;
		int windowHeight = 1080;

		bool showDebugInfo = true;
		bool showLevelEditor = true;
		bool showHierarchy = true;
		bool showInspector = true;
		bool showGameViewport = true;
		bool showAssetBrowser = true;
		bool showToolbar = true;
		bool showPreferencesWindow = false;
		bool showProjectSettingsWindow = false;
		bool showPrefabEditor = false;
		bool showLayerManager = true;
		bool showAnimationEditor = false;
	};

	/*********************************************************************//**
	 * \brief   Enumeration defining the different categories of prefabs
	 *          available for instantiation in the editor.
	 *********************************************************************/
	enum class PrefabType {
		None,
		Eraser,
		Platform,
		TrapPlatform,
		Tile
	};

	/*********************************************************************//**
	 * \brief   Structure holding information about an interactive prefab
	 *          item used within the editor's drag-and-drop palette system.
	 *********************************************************************/
	struct DraggablePrefabInfo {
		std::string name;
		PrefabType type = PrefabType::None;
		Framework::Texture textureID = 0; // OpenGL Texture ID
		// tex coords(uses dearimgui vec struct)
		ImVec2 uv0{ 0.0f, 0.0f };
		ImVec2 uv1{ 1.0f, 1.0f };
	};

	/*********************************************************************//**
	 * \brief   Helper structure utilized to link a tile name with its
	 *          associated texture ID string.
	 *********************************************************************/
	struct TileData {
		std::string name;
		std::string textureID;
	};

	/*********************************************************************//**
	 * \brief   Structure holding configuration for Finite State Machine (FSM)
	 *          templates that can be dragged and dropped into the scene.
	 *********************************************************************/
	struct DraggableFSMInfo {
		std::string name;
		std::string fsmPath;
		std::string textureKey;
		ImTextureID textureID = 0;
		ImVec2 uv0;
		ImVec2 uv1;
		ImVec2 halfSize{ 25.0f, 25.0f };
	};

	/*********************************************************************//**
	 * \brief   Manages an active editing state on a specific component,
	 *          capturing initial and final JSON representations for the
	 *          undo/redo history tracking system.
	 *********************************************************************/
	struct EditSession {
		bool active = false;
		bool wasItemActive = false;
		Entity entityInEdit = 0;
		Framework::ComponentType compInEdit = Framework::ComponentType::MAX_COMPONENTS;
		std::string beforeJSON;

		// Marks the beginning of the editing session for a component, saving it's initial state.
		void Begin(Entity entity, Framework::ComponentType compType);
		// Marks the end of the editing session for a component, saving it's post modification state.
		void End(Entity entity, Framework::ComponentType compType, const std::string& description, HistoryManager& historyManagerRef);
		// Clears Session Data.
		void Flush();
	};

	/*********************************************************************//**
	 * \class   Editor
	 * \brief   The main level editor system. Integrates with Dear ImGui to
	 *          provide tools for scene construction, entity manipulation,
	 *          asset management, and real-time debugging.
	 *********************************************************************/
	class Editor : public Framework::ISystem {
	private:
		// --- System Pointers ---
		Framework::CoreEngine* engine_ = nullptr; // Enforce that the Editor must be instantiated after engine
		Framework::GameManager* gameManager_ = nullptr; // Enforce that the Editor must be instantiated after GameManager
		Framework::CameraSystem* cameraSystem_ = nullptr; // Pointer to camera systems for level editor
		Framework::Graphics* graphicsSystem_ = nullptr; //  Pointer to Graphics system
		Framework::ResourceManager* resourceManager_ = nullptr; // Pointer to ResourceManager for asset loading
		Framework::TransformationSystem* transformationSystem_ = nullptr;

		// --- Codependent Systems ---
		HistoryManager historyManager_; // HistoryManager for Undo/Redo System.
		GizmoSystem* gizmoSystem_ = nullptr;
		BuildSizeAnalyzer buildSizeAnalyzer_;

		// --- Internal Properties ---
		Entity editorCamera_ = 0;
		float editorZoom_ = 1.0f;
		Vector2D editorCamPos_ = { 0.0f, 0.0f };
		bool isEditorFocused_ = true; // toggled when ImGui viewport or editor window is active

		AssetBrowser assetBrowser;

		// --- Internal State ---
		bool initialized = false;
		bool firstUpdate = true;
		bool isGamePlaying = false;
		bool isGamePaused = false;
		//  Members for the system profiler display
		float profilerUpdateTimer_ = 0.0f;
		std::vector<Framework::SystemTime> profilerDisplayData_;

		// Entity Inspector
		EditSession editSession;
		int selectedEntityIndex = 0;  // DEPRECATED: Keep for backward compatibility during transition
		std::vector<Entity> selectedEntities_;  // New: Support multiple entity selection
		Entity lastClickedEntity_ = 0;  // Track last clicked entity for range selection
		std::vector<std::string> componentNames;
		std::vector<const char*> componentItems;
		std::vector<Framework::ComponentType> componentTypes;
		int selectedComponentIndex = 0;
		std::vector<std::string> tagNames;
		std::vector<const char*> tagItems;
		std::vector<Framework::ComponentType> tagTypes;
		int selectedTagIndex = 0;
		// Native Scripting
		int selectedScriptIndex = 0;
		int selectedReplaceScriptIndex = 0;

		// --- Tilemap Editor State ---
		bool isTileEditorActive_ = false; //  Flag to control tile editor mode
		bool showGrid_ = false;
		float baseGridCellSize_ = 100.f;   // NEW: World-space size of a cell at zoom 1.0

		//Drag and Drop State
		bool isDraggingPrefab_ = false;
		DraggablePrefabInfo dragDropPayload_; // Holds info about the prefab being dragged
		Vector2D dragDropPreviewPos_; // World position for preview rendering

		// Tracks whether move game object mode is activated. Toggled by Toggle Move button on the toolbar, for in world game obj drag and drop
		bool isDraggingGameObj_ = false; 

		// Brush mode for continous placement
		bool isBrushMode_ = false;
		DraggablePrefabInfo brushPayload_; // Stores the item selected for brushing
		Vector2D brushPreviewPos_;         // World position for brush preview
		Vector2D lastPlacedPos_ = { -99999.0f, -99999.0f }; // To prevent stacking duplicates on same frame

		//Palette Info storage (prefabs)
		bool palettePrepared_ = false; // flag for storing palette items
		std::vector<DraggablePrefabInfo> paletteItems_;

		//tiles
		bool tilePalettePrepared_ = false;
		bool isTilePaletteOpen_ = false;
		std::vector<DraggablePrefabInfo> tilePaletteItems_;
		std::vector<TileData> tileDataList_;

		//---layer manager variables ----//
		Framework::LayerManager layerManager_;

		// Layer editor state
		bool showLayerConfigWindow_ = false;
		int selectedRenderLayerIdx_ = -1;
		int selectedCollisionLayerIdx_ = -1;
		char newRenderLayerName_[128] = "New Layer";
		int newRenderLayerOrder_ = 0;
		char newCollisionLayerName_[128] = "New Layer";

		// Game viewport tracking for proper mouse coordinate transformation
		ImVec2 gameViewportPos_ = { 0, 0 };    // Top-left position of game viewport in screen space
		ImVec2 gameViewportSize_ = { 0, 0 };   // Size of game viewport
		bool isMouseOverGameViewport_ = false;
		//void DrawEditorGridGfx();

		// Entity factory System members
		Framework::PrefabSystem prefabSystem_; // Each editor instance has its own prefab system
		PrefabType selectedPrefabType_ = PrefabType::None;
		std::string selectedPrefabName_;

		// --- Entity Selection & Deletion ---
		bool isEntitySelected_ = false;  // Flag to track if an entity is selected
		std::vector<std::string> entitiesClipboard_;  // Support multiple entities in clipboard
		std::unordered_map<Framework::SceneGUID, std::vector<std::string>> scenePreloadTextures_;
		std::unordered_set<Framework::SceneGUID> scenePreloadLoaded_;
		char preloadTextureInput_[256] = "";
		bool isMarqueeMode_ = false; // I key toggles marquee selection mode
		bool isMarqueeSelecting_ = false;
		ImVec2 marqueeStartScreenPos_ = { 0.0f, 0.0f };
		ImVec2 marqueeEndScreenPos_ = { 0.0f, 0.0f };

		// Drag and Drop Support (Windows to Editor)
		std::vector<std::filesystem::path> droppedFilePaths_;
		static void DropCallback(GLFWwindow* window, int count, const char* paths[]);

		// FSM Editor state
		bool isFSMEditorOpen_ = false;
		std::string selectedFSMDefinition_;
		std::vector<std::string> availableFSMDefinitions_;
		bool fsmPalettePrepared_ = false;

		std::vector<DraggableFSMInfo> fsmPaletteItems_;
		DraggableFSMInfo fsmDragDropPayload_;
		bool isDraggingFSM_ = false;

		// --- Camera State Management ---
		struct CameraState {
			Vector2D position{ 0.0f, 0.0f };
			float zoom = 1.0f;
		};

		CameraState defaultEditorCameraState_{ {0.0f, 0.0f}, 1.0f };
		CameraState currentEditorCameraState_;
		bool editorCameraNeedsReset_ = false;

		// -------------------------------------------------------
		// --- Project Management State (New / Import Project) ---
		// -------------------------------------------------------
		bool showNewProjectDialog_ = false;  // Controls "Create New Project" modal visibility
		bool showImportProjectDialog_ = false;  // Controls "Import Existing Project" modal visibility
		char newProjectNameBuffer_[256] = "";  // Text input buffer for the new project name
		char newProjectPathBuffer_[512] = "";  // Root directory where the new project folder will be created
		char importProjectPathBuffer_[512] = ""; // Path to the folder of an existing project to import
		std::string projectFeedbackMsg_ = "";  // Feedback message shown inside the modal after an action
		bool projectActionSucceeded_ = false; // Whether the last project action succeeded (colours the message)

		/*********************************************************************//**
		 * \brief   Renders the "Create New Project" modal pop-up.
		 *********************************************************************/
		void ShowNewProjectDialog();

		/*********************************************************************//**
		 * \brief   Renders the "Import Existing Project" modal pop-up.
		 *********************************************************************/
		void ShowImportProjectDialog();
		// -------------------------------------------------------

		/*********************************************************************//**
		 * \brief   Resets the editor camera to its standard initial configurations.
		 *********************************************************************/
		void ResetEditorCamera();

		/*********************************************************************//**
		 * \brief   Saves the current position and zoom state of the editor camera.
		 *********************************************************************/
		void SaveEditorCameraState();

		/*********************************************************************//**
		 * \brief   Restores the previously saved position and zoom state of the
		 *          editor camera.
		 *********************************************************************/
		void RestoreEditorCameraState();

		// --- Private Helper Functions ---

		/*********************************************************************//**
		 * \brief   Validates the state of the editor dependencies.
		 *********************************************************************/
		void SanityCheck();

		/*********************************************************************//**
		 * \brief   Loads the editor configurations.
		 *********************************************************************/
		void LoadEditorConfig();

		/*********************************************************************//**
		 * \brief   Serializes and saves current editor configurations.
		 *********************************************************************/
		void SaveEditorConfig();

		/*********************************************************************//**
		 * \brief   Handles placing the dragged prefab when the mouse is released.
		 *********************************************************************/
		void HandleDragDropPlacement();

		/*********************************************************************//**
		 * \brief   Renders a visual preview of the prefab being dragged.
		 *
		 * \param   drawList
		 *********************************************************************/
		void DrawDragDropPreview(ImDrawList* drawList);

		/*********************************************************************//**
		 * \brief   Handles erasing entities from the grid when the eraser tool
		 *          is active in Brush Mode.
		 *********************************************************************/
		void HandleTileErasing();

		/*********************************************************************//**
		 * \brief   Handles the instantiation logic for spawning a selected
		 *          prefab at the mouse cursor position.
		 *********************************************************************/
		void HandleSpawning();

		/*********************************************************************//**
		 * \brief   Handles logic for selecting an entity by clicking on it
		 *          in the game world or using Marquee selection.
		 *********************************************************************/
		void HandleSelection();

		/*********************************************************************//**
		 * \brief   Handles logic for deleting selected entities when the
		 *          Delete key is pressed.
		 *********************************************************************/
		void HandleDeletion();

		/*********************************************************************//**
		 * \brief   Handles input specifics when the game viewport is focused.
		 *********************************************************************/
		void HandleGameViewportInput();

		/* Entity Copy & Pasting APIs */

		/*********************************************************************//**
		 * \brief   Copies a single entity's JSON representation to the clipboard.
		 *
		 * \param   entity
		 *********************************************************************/
		void CopyEntityToClipboard(const Entity entity);

		/*********************************************************************//**
		 * \brief   Copies a vector of entities to the clipboard.
		 *
		 * \param   entities
		 *********************************************************************/
		void CopyEntitiesToClipboard(const std::vector<Entity>& entities);

		/*********************************************************************//**
		 * \brief   Pastes an entity from the clipboard data into the scene.
		 *********************************************************************/
		void PasteEntityFromClipboard();

		/*********************************************************************//**
		 * \brief   Pastes multiple entities from the clipboard into the scene.
		 *********************************************************************/
		void PasteEntitiesFromClipboard();

		/*********************************************************************//**
		 * \brief   Serializes an entity to a string format.
		 *
		 * \param   entity
		 * \return  A JSON string representation of the entity.
		 *********************************************************************/
		std::string SerializeEntityToString(const Entity entity);

		// --- GUI Panel Functions ---

		/*********************************************************************//**
		 * \brief   Renders the Performance Profiler panel.
		 *********************************************************************/
		void ShowPerformanceProfile();

		/*********************************************************************//**
		 * \brief   Renders the main Level Editor controls panel.
		 *********************************************************************/
		void ShowLevelEditor();

		/*********************************************************************//**
		 * \brief   Renders the Save and Load UI section within the Level Editor.
		 *********************************************************************/
		void SaveAndLoadSection();

		/*********************************************************************//**
		 * \brief   Renders the Scene Hierarchy panel.
		 *********************************************************************/
		void ShowHierarchy();

		/*********************************************************************//**
		 * \brief   Renders the Entity Inspector panel to edit components.
		 *********************************************************************/
		void ShowInspector();

		/*********************************************************************//**
		 * \brief   Renders the specific data block for a selected component.
		 *
		 * \param   selectedEntity
		 * \param   selectedComponent
		 *********************************************************************/
		void ShowComponentSections(const Entity& selectedEntity, const Framework::ComponentType& selectedComponent);

		/*********************************************************************//**
		 * \brief   Renders the Tilemap Editor panel.
		 *********************************************************************/
		void ShowTilemapEditor();

		/*********************************************************************//**
		 * \brief   Renders the Asset Browser panel.
		 *
		 * \param   droppedFiles
		 *********************************************************************/
		void ShowAssetBrowser(std::vector<std::filesystem::path>& droppedFiles);

		/*********************************************************************//**
		 * \brief   Renders the Game Viewport panel.
		 *********************************************************************/
		void ShowGameViewport();

		/*********************************************************************//**
		 * \brief   Renders the top application Menu Bar.
		 *********************************************************************/
		void ShowMenuBar();

		/*********************************************************************//**
		 * \brief   Renders the Toolbar panel for quick actions.
		 *********************************************************************/
		void ShowToolbar();

		/*********************************************************************//**
		 * \brief   Renders the Preferences modal window.
		 *********************************************************************/
		void ShowPreferencesWindow();

		/*********************************************************************//**
		 * \brief   Renders the Project Settings modal window.
		 *********************************************************************/
		void ShowProjectSettingsWindow();

		/*********************************************************************//**
		 * \brief   Renders the Debug Visualization overlay panel.
		 *********************************************************************/
		void ShowDebugVisualizationPanel();

		/*********************************************************************//**
		 * \brief   Renders the Prefab Editor panel.
		 *********************************************************************/
		void ShowPrefabEditor();

		/*********************************************************************//**
		 * \brief   Renders the Tile Palette selector panel.
		 *********************************************************************/
		void ShowTilePalette();

		/*********************************************************************//**
		 * \brief   Renders the Layer Manager panel for editing layers.
		 *********************************************************************/
		void ShowLayerManager();

		// Component-specific inspector sub-panels
		void DrawTransformInspector(Entity entity);
		void DrawRigidBodyInspector(Entity entity);
		void DrawTextureInspector(Entity entity);
		void DrawAnimationInspector(Entity entity);
		void DrawAudioInspector(Entity entity);
		void DrawButtonInspector(Entity entity);
		void DrawCameraInspector(Entity entity);

		// Popup Related
		std::string popupMsg;

		// --- Grid Helper Functions ---

		/*********************************************************************//**
		 * \brief   Draws the world grid when the tile editor is active.
		 *
		 * \param   drawList
		 *********************************************************************/
		void DrawEditorGrid(ImDrawList* drawList);

		std::string selectedPrefabForEditing_;
		PrefabType selectedPrefabTypeForEditing_ = PrefabType::None;
		bool isPrefabEditorOpen_ = false;

		// Helper functions for prefabs 

		/*********************************************************************//**
		 * \brief   Applies modified prefab configurations to active instances.
		 *
		 * \param   prefabName
		 *********************************************************************/
		void ApplyPrefabChangesToEntities(const std::string& prefabName);

		/*********************************************************************//**
		 * \brief   Serializes and saves prefab changes to the disk.
		 *
		 * \param   prefabName
		 *********************************************************************/
		void SavePrefabToFile(const std::string& prefabName);

		/*********************************************************************//**
		 * \brief   Iterates the active scene and updates all recognized prefabs.
		 *********************************************************************/
		void ApplyAllPrefabChangesToCurrentScene();

		// Animation Editor Helper functions
		void ShowAnimationEditor();
		void LoadAnimationFiles();
		void SaveAnimationFile(const std::string& animationName);
		void SaveTextureToAnimationMapping();
		void ReloadAnimationFromFile(const std::string& animationName);
		void RefreshAnimationInResourceManager(const std::string& animationName);

		// Scene Management Helper functions

		/*********************************************************************//**
		 * \brief   Serializes and saves the active scene to disk.
		 *********************************************************************/
		void SaveScene();

		/*********************************************************************//**
		 * \brief   Renders the Scene Asset Preload section for textures.
		 *********************************************************************/
		void DrawScenePreloadTexturesUI(const Framework::SceneAsset& currentScene);

		/*********************************************************************//**
		 * \brief   Determines the object located under the current mouse position.
		 *
		 * \return  The entity ID at the mouse coordinate, or 0 if none.
		 *********************************************************************/
		Entity GetObjAtMousePos();

		/*********************************************************************//**
		 * \brief   Prepares texture resources for general palette platform items.
		 *********************************************************************/
		void PreparePaletteItems();

		/*********************************************************************//**
		 * \brief   Prepares texture resources specifically for tile items.
		 *********************************************************************/
		void PrepareTilePaletteItems();

		/*********************************************************************//**
		 * \brief   Instantiates the dragged object into the active scene.
		 *
		 * \param   info
		 * \param   position
		 *********************************************************************/
		void PlaceObjectInWorld(const DraggablePrefabInfo& info, Vector2D position);

		/*********************************************************************//**
		 * \brief   Calculates the half-size extent of a given prefab.
		 *
		 * \param   name
		 * \param   type
		 * \return  A vector holding the X and Y half size extents.
		 *********************************************************************/
		Vector2D GetPrefabHalfSize(const std::string& name, PrefabType type);

		/*********************************************************************//**
		 * \brief   Handles painting placement input behavior for brushes.
		 *********************************************************************/
		void HandleBrushInput();

		/*********************************************************************//**
		 * \brief   Updates positional state of the Game Viewport within ImGui.
		 *********************************************************************/
		void UpdateViewportData();

		/*********************************************************************//**
		 * \brief   Sorts vector of entities to accurately determine rendering priority.
		 *
		 * \param   entities
		 * \param   entityMgr
		 *********************************************************************/
		void SortEntitiesByRenderLayer(std::vector<Entity>& entities, Framework::EntityManager& entityMgr);


		// FSM DRAGGABLE HELPER FUNCTIONS
		void ShowFSMEditor();
		void PrepareFSMPaletteItems();
		void HandleFSMDragDropPlacement();
		void LoadFSMDefinitionsFromFolder(const std::string& folderPath);
		Entity CreateFSMEntityAtPosition(const std::string& fsmPath, const Vector2D& position);

		// Layer viewport data for IMGUI
		void ShowCollisionMatrix(Framework::PhysicsSystem* physics);
		void ShowEntityCollisionLayer(Framework::EntityManager& entityMgr, Entity selectedEntity);
		void ShowEntityRenderLayer(Framework::EntityManager& entityMgr, Entity selectedEntity);
		void ShowSceneEntityTracker(Framework::EntityManager& entityMgr);
		void ShowLayerConfigWindow();
		void ShowRenderLayersTab();
		void ShowCollisionLayersTab();


		// --- Animation Editor State ---
		struct AnimationFileData {
			std::string name;
			int frameCount = 0;
			float frameDuration = 0.1f;
			bool isLooping = true;
			int startFrame = 0;
			std::vector<std::pair<std::string, int>> spritesheets; // texture, frameCount pairs
			std::vector<std::string> textureKeys; // keys from textureToAnimationMapping.json
			std::map<int, std::string> frameEvents; // maps frame numbers to event names for quick lookup when editing events on specific frames
		};

		std::vector<AnimationFileData> loadedAnimations_;
		int selectedAnimationIndex_ = 0;
		std::string selectedAnimationName_;
		std::vector<const char*> animationNames_;
		bool isAnimationEditorInitialized_ = false;
		bool showCreatePrefabDialog_ = false;
		char newPrefabNameBuffer_[256] = "";
		Entity entityForPrefabCreation_ = 0;

		/*********************************************************************//**
		 * \brief   Helper prompt providing dialog for creating new prefabs.
		 *********************************************************************/
		void ShowCreatePrefabDialog();

	public:
		bool isEnabled = true;
		EditorSettings settings;

		/* Constructors */
		// Editors should not be cloneable or assignable. Only 1 should ever exist on the engine.
		Editor(const Editor&) = delete;
		Editor& operator=(const Editor&) = delete;

		Editor(Framework::CoreEngine& engine, Framework::GameManager& gameManager, Framework::ResourceManager& rcm);

		// Editor loop functions
		void Initialize() override;
		void Update() override;
		void Shutdown() override;

		/* Helper Functions for playing / pausing state */
		const bool IsGamePlaying() const { return isGamePlaying; };
		const bool IsGamePaused() const { return isGamePaused; };
		const ImVec2 GetGameViewportSize() const { return gameViewportSize_; }
		const ImVec2 GetGameViewportPos() const { return gameViewportPos_; }

		/*********************************************************************//**
		 * \brief   Snaps a given continuous position stringently to the Editor Grid.
		 *
		 * \param   worldPos
		 * \param   normalized
		 * \return  The floored/snapped vector location.
		 *********************************************************************/
		Vector2D SnapToGrid(const Vector2D& worldPos, bool normalized = false) const;

		/*********************************************************************//**
		 * \brief   Converts internal screen coordinates into the true continuous
		 *          mouse game-space position representing the camera viewport.
		 * \return  World units position inside the game world.
		 *********************************************************************/
		Vector2D GetMouseWorldPosition();

		/*********************************************************************//**
		 * \brief   Retrieves the current mouse position in Normalized Device Coordinates (NDC) relative to the game viewport limits.
		 * \return  A Vector2D representing normalized bounds.
		 *********************************************************************/
		Vector2D GetNormalizedMousePos() const;

		/*********************************************************************//**
		 * \brief   Converts world-space to ImGui screen positions with a cam ID.
		 *
		 * \param   worldPos
		 * \param   cameraID
		 * \return  The coordinate vector matching the ImGui drawing context window.
		 *********************************************************************/
		ImVec2 WorldToImGuiScreen(const Vector2D& worldPos, Entity cameraID);

		/*********************************************************************//**
		 * \brief   Converts world-space to ImGui screen positions via default editor camera.
		 *
		 * \param   worldPos
		 * \return  The translated ImGui window drawing offset.
		 *********************************************************************/
		ImVec2 WorldToImGuiScreen(const Vector2D& worldPos);

		/*********************************************************************//**
		 * \brief   Converts normalized coordinates to ImGui screen-space.
		 *
		 * \param   normalizedPos
		 * \return  The absolute screen X/Y.
		 *********************************************************************/
		ImVec2 NormalizedToImGuiScreen(const Vector2D& normalizedPos) const;

		/*********************************************************************//**
		 * \brief   Setter for active target Gizmos tracking controller system reference.
		 *********************************************************************/
		void SetGizmoSystem(GizmoSystem* gizmo) {
			gizmoSystem_ = gizmo;
		}

		/*********************************************************************//**
		 * \brief   Getter for the editor operational undo/redo tracker log.
		 *********************************************************************/
		HistoryManager* GetHistoryMgr() { return &historyManager_; }
	};
}
