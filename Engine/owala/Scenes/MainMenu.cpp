/*****************************************************************//**
 * \file   MainMenu.cpp
 * \brief  Implements a simple main menu with Play, Settings, Quit.
 *
 * Buttons are defined in JSON and wired here via ButtonClickedEvent.
 *
 * \author  Cello + ChatGPT helper :)
 *********************************************************************/
#include "MainMenu.h"

 // Engine
#include "EntityManager.h"
#include "SceneManager.h"
#include "EntityStorage.h"
#include "InputSystem.h"
#include "Graphics.h"
#include "ResourceManager.h"
#include "GuiSystem.h"
#include "MessageBus.h"
#include "GameMessages.h"
#include "GameManager.h"

// Standard Library
#include <iostream>
#include <string>

// Pointers to engine systems
Framework::Font* menuFont_ = nullptr;
Framework::EntityStorage* entityStorage_ = nullptr;
// Input system pointer (for ESC key)
Framework::InputSystem* inputSystem_ = nullptr;

// Main menu entities
Entity menuBackground, menuFrame, menuTitle;
Entity playButton, playButtonText;
Entity settingsButton, settingsButtonText;
Entity quitButton, quitButtonText;

// State
bool menuCreated = false;

// Actions queued from button clicks
bool pendingPlay = false;
bool pendingSettings = false;
bool pendingQuit = false;

// Settings overlay entities
Entity settingsPanel = 0;
Entity settingsCancelButton = 0;
Entity settingsCancelButtonText = 0;

// Settings state
bool settingsOpen = false;
bool pendingCloseSettings = false;

// Pause menu overlay entities
Entity pausePanel = 0;
Entity pauseResumeButton = 0;
Entity pauseResumeButtonText = 0;

// Pause state
bool pauseOpen = false;
bool pendingPauseOpen = false;
bool pendingPauseClose = false;

namespace MainMenu {

	using namespace Framework;

	void HandleButtonClick(const ButtonClickedEvent& event) {
		EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
		std::string& buttonName = entityMgr.GetProperties(event.button).name;

		// 🔒 If settings overlay is open, ignore all buttons except the Cancel button
		if (settingsOpen && buttonName != "SettingsCancelButton") {
			std::cout << "[MainMenu] Click on '" << buttonName
				<< "' ignored because Settings is open.\n";
			return;
		}

		// 🔒 If pause menu is open, ignore all buttons except the Resume button
		if (pauseOpen && buttonName != "PauseResumeButton") {
			std::cout << "[MainMenu] Click on '" << buttonName
				<< "' ignored because Pause Menu is open.\n";
			return;
		}

		std::cout << "[MainMenu] Button clicked: " << buttonName << std::endl;

		if (buttonName == "PlayButton") {
			pendingPlay = true;
		}
		else if (buttonName == "SettingsButton") {
			pendingSettings = true;
		}
		else if (buttonName == "SettingsCancelButton") {
			pendingCloseSettings = true;
		}
		else if (buttonName == "PauseResumeButton") {
			pendingPauseClose = true;    // Resume = close pause menu
		}
		else if (buttonName == "QuitGameButton") {
			pendingQuit = true;
		}
	}

	void CreateMainMenu() {
		EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
		SceneManager& sceneMgr = SceneManager::GetInstance();
	}

	void CleanupMenu() {
		EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);

		auto destroyIfValid = [&](Entity& e) {
			if (e != 0) {
				entityMgr.DestroyEntity(e);
				e = 0;
			}
			};

		destroyIfValid(menuBackground);
		destroyIfValid(menuFrame);
		destroyIfValid(menuTitle);

		destroyIfValid(playButton);
		destroyIfValid(playButtonText);
		destroyIfValid(settingsButton);
		destroyIfValid(settingsButtonText);
		destroyIfValid(quitButton);
		destroyIfValid(quitButtonText);

		menuCreated = false;
	}

	// Creates the Settings overlay (panel + Cancel button)
	void CreateSettingsOverlay() {
		if (settingsOpen) return; // already visible

		EntityManager entityMgr(*entityStorage_);
		SceneManager& sceneMgr = SceneManager::GetInstance();

		// 1) Big Settings PNG in the center
		settingsPanel = entityMgr.CreateEntity();
		entityMgr.GetProperties(settingsPanel).name = "SettingsPanel";

		// Centered on screen
		entityMgr.AddComponent(settingsPanel, Position{ { 0.0f, 0.0f } });

		// Uses your texture: Assets/Sprites/UI/Main_Menu/Settings.png
		// ResourceManager only needs the filename "Settings.png"
		entityMgr.AddComponent(settingsPanel, TextureComponent{
			"Settings.png",          // texture name
			{ 0.8f, 0.8f }           // scale (adjust if too big/small)
			});

		entityMgr.AddComponent(settingsPanel, GUITag{});
		entityMgr.AddComponent(settingsPanel, ActiveTag{});
		sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), settingsPanel);

		// 2) Cancel button on top of the panel
		settingsCancelButton = entityMgr.CreateEntity();
		entityMgr.GetProperties(settingsCancelButton).name = "SettingsCancelButton";

		// Slightly below center (move up/down by changing -0.25f)
		entityMgr.AddComponent(settingsCancelButton, Position{ { 0.0f, -0.35f } });

		// Reuse your existing Button.png sprite
		entityMgr.AddComponent(settingsCancelButton, TextureComponent{
			"Button.png",
			{ 0.25f, 0.08f }
			});

		entityMgr.AddComponent(settingsCancelButton, Button{});
		entityMgr.AddComponent(settingsCancelButton, GUITag{});
		entityMgr.AddComponent(settingsCancelButton, ActiveTag{});
		sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), settingsCancelButton);

		// 3) Text that says "Cancel" on top of the Cancel button
		settingsCancelButtonText = entityMgr.CreateEntity();
		entityMgr.GetProperties(settingsCancelButtonText).name = "SettingsCancelButtonText";

		// Small X offset so text looks centered (tweak -0.07f if needed)
		entityMgr.AddComponent(settingsCancelButtonText, Position{ { -0.06f, -0.38f } });

		entityMgr.AddComponent(settingsCancelButtonText, GUITag{});
		entityMgr.AddComponent(settingsCancelButtonText, TextComponent{
			"Cancel",                     // text
			"Amethysta-Regular.ttf",      // same font as other buttons
			{ 1.0f, 1.0f, 1.0f, 1.0f },   // white color
			0.9f                           // scale
			});
		sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), settingsCancelButtonText);

		settingsOpen = true;
	}

	// Destroys the overlay again
	void DestroySettingsOverlay() {
		if (!settingsOpen) return;

		EntityManager entityMgr(*entityStorage_);

		auto destroyIfValid = [&](Entity& e) {
			if (e != 0) {
				entityMgr.DestroyEntity(e);
				e = 0;
			}
		};

		destroyIfValid(settingsPanel);
		destroyIfValid(settingsCancelButton);
		destroyIfValid(settingsCancelButtonText);

		settingsOpen = false;
	}

	// Creates the Pause Menu overlay (Basic Pause Menu + Resume button)
	void CreatePauseOverlay() {
		if (pauseOpen) return; // already visible

		EntityManager entityMgr(*entityStorage_);
		SceneManager& sceneMgr = SceneManager::GetInstance();

		// 1) Big Pause PNG in the center
		pausePanel = entityMgr.CreateEntity();
		entityMgr.GetProperties(pausePanel).name = "PausePanel";

		// Centered on screen
		entityMgr.AddComponent(pausePanel, Position{ { 0.0f, 0.0f } });

		// Uses your texture: Assets/Sprites/UI/Main_Menu/Basic Pause Menu.png
		// ResourceManager only needs the filename as the key
		entityMgr.AddComponent(pausePanel, TextureComponent{
			"Basic Pause Menu.png",   // texture key (matches filename)
			{ 0.8f, 0.8f }            // scale; tweak if too big/small
			});

		entityMgr.AddComponent(pausePanel, GUITag{});
		entityMgr.AddComponent(pausePanel, ActiveTag{});
		sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), pausePanel);

		// 2) Resume button at (0, 0) as you requested
		pauseResumeButton = entityMgr.CreateEntity();
		entityMgr.GetProperties(pauseResumeButton).name = "PauseResumeButton";

		entityMgr.AddComponent(pauseResumeButton, Position{ { 0.0f, 0.0f } });

		// Reuse your existing Button.png sprite
		entityMgr.AddComponent(pauseResumeButton, TextureComponent{
			"Button.png",
			{ 0.25f, 0.08f }
			});

		entityMgr.AddComponent(pauseResumeButton, Button{});
		entityMgr.AddComponent(pauseResumeButton, GUITag{});
		entityMgr.AddComponent(pauseResumeButton, ActiveTag{});
		sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), pauseResumeButton);

		// 3) Text that says "Resume" on top of the button
		pauseResumeButtonText = entityMgr.CreateEntity();
		entityMgr.GetProperties(pauseResumeButtonText).name = "PauseResumeButtonText";

		// Slight X offset so text looks centered on the button
		entityMgr.AddComponent(pauseResumeButtonText, Position{ { -0.07f, 0.0f } });

		entityMgr.AddComponent(pauseResumeButtonText, GUITag{});
		entityMgr.AddComponent(pauseResumeButtonText, TextComponent{
			"Resume",                    // text
			"Amethysta-Regular.ttf",     // same font as other buttons
			{ 1.0f, 1.0f, 1.0f, 1.0f },  // white
			0.9f                         // scale
			});
		sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), pauseResumeButtonText);

		pauseOpen = true;
	}

	// Destroys the Pause Menu overlay again
	void DestroyPauseOverlay() {
		if (!pauseOpen) return;

		EntityManager entityMgr(*entityStorage_);

		auto destroyIfValid = [&](Entity& e) {
			if (e != 0) {
				entityMgr.DestroyEntity(e);
				e = 0;
			}
		};

		destroyIfValid(pausePanel);
		destroyIfValid(pauseResumeButton);
		destroyIfValid(pauseResumeButtonText);

		pauseOpen = false;
	}

	void Initialize() {
		
		GameManager& gm = GameManager::GetInstance();

		// store ECS pointer so CreateSettingsOverlay/DestroySettingsOverlay can use it
		entityStorage_ = gm.GetSystems().ecs_;

		// store InputSystem pointer for ESC key
		inputSystem_ = gm.GetSystems().input_;
		
		ResourceManager& resourceManager_ = *GameManager::GetInstance().GetSystems().resourceManager_;

		// Load font ...
		try {
			menuFont_ = &resourceManager_.get<Font>("Amethysta-Regular.ttf");
		}
		catch (...) {
			std::cerr << "Warning: MainMenu font not loaded." << std::endl;
			menuFont_ = nullptr;
		}

		// 🔹 Subscribe to button click events
		MessageBus::Instance().Subscribe<ButtonClickedEvent>(
			[](const ButtonClickedEvent& e) { HandleButtonClick(e); }
		);

		CreateMainMenu();
		menuCreated = true;
		std::cout << "Main Menu Initialized!!!" << std::endl;
	}

	void Update() {
		
		// 1) Handle ESC key to toggle pause menu
		if (inputSystem_) {
			if (inputSystem_->KeyPressed(InputSystem::Key::Escape)) {
				if (!pauseOpen && !settingsOpen) {
					// Open pause menu
					pendingPauseOpen = true;
				}
				else if (pauseOpen) {
					// If already paused, ESC can also resume
					pendingPauseClose = true;
				}
			}
		}
		// 2) Handle any queued button actions

		if (pendingPlay) {
			std::cout << "[MainMenu] PLAY selected. Loading 'Splash Screen'..." << std::endl;
			
			// Ask GameManager to open "<Name Of Scene>".
			GameManager::GetInstance().ChangeScene("Splash Screen");

			pendingPlay = false;
		}

		// When Settings button is pressed → show overlay
		if (pendingSettings) {
			std::cout << "[MainMenu] SETTINGS selected." << std::endl;

			if (!settingsOpen) {
				CreateSettingsOverlay();
			}

			pendingSettings = false;
		}

		// NEW: when Cancel is pressed → hide overlay
		if (pendingCloseSettings) {
			std::cout << "[MainMenu] SETTINGS CANCEL selected." << std::endl;

			if (settingsOpen) {
				DestroySettingsOverlay();
			}

			pendingCloseSettings = false;
		}

		// Pause open
		if (pendingPauseOpen) {
			std::cout << "[MainMenu] PAUSE MENU opened." << std::endl;
			if (!pauseOpen) {
				CreatePauseOverlay();
			}
			pendingPauseOpen = false;
		}

		// Pause close (Resume)
		if (pendingPauseClose) {
			std::cout << "[MainMenu] RESUME selected." << std::endl;
			if (pauseOpen) {
				DestroyPauseOverlay();
			}
			pendingPauseClose = false;
		}

		if (pendingQuit) {
			std::cout << "[MainMenu] QUIT GAME selected." << std::endl;

			// Ask GameManager to close the application window
			GameManager::GetInstance().RequestQuit();

			pendingQuit = false;
		}
	}

	void Free() {
		if (menuCreated) {
			CleanupMenu();
		}

		if (settingsOpen) {
			DestroySettingsOverlay();
		}

		if (pauseOpen) {
			DestroyPauseOverlay();
		}
	}
}