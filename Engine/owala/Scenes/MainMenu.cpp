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
#include "AudioSystem.h"   // 👈 ADD THIS

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
Entity howToPlayButton, howToPlayButtonText;
Entity settingsButton, settingsButtonText;
Entity quitButton, quitButtonText;

// State
bool menuCreated = false;

// Actions queued from button clicks
bool pendingPlay = false;
bool pendingHowToPlay = false;  // NEW
bool pendingSettings = false;
bool pendingQuit = false;

// --- HOW TO PLAY overlay state ---
Entity howToPlayPanel = 0;  // big image panel
Entity howToPlayLeftArrow = 0;  // left arrow button
Entity howToPlayRightArrow = 0;  // right arrow button;

bool howToPlayOpen = false;
int  howToPlayPageIndex = 0;

constexpr int kHowToPlayPageCount = 3; // pages 0..3

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

	// How To Play overlay helpers
	void OpenHowToPlayOverlay();
	void CloseHowToPlayOverlay();
	void UpdateHowToPlayTexture();

	// -----------------------------------------------------------------------------
    // Small helpers to play UI SFX
    // -----------------------------------------------------------------------------
	namespace
	{
		// Plays generic UI click SFX using a pre-loaded sound in ResourceManager
		void PlayClickSFX()
		{
			GameManager& gm = GameManager::GetInstance();
			auto* audioSystem = gm.GetSystems().audio_;
			auto* resourceMgr = gm.GetSystems().resourceManager_;
			if (!audioSystem || !resourceMgr) return;

			try {
				FMOD::Sound& sfx = resourceMgr->get<FMOD::Sound>("UI_Button_Clicked_02");
				audioSystem->play(sfx, 0.9f, false);
			}
			catch (...) {
				std::cerr << "[MainMenu] Missing SFX: UI_Button_Clicked_02\n";
			}
		}

		// Plays cancel / back SFX
		void PlayCancelSFX()
		{
			GameManager& gm = GameManager::GetInstance();
			auto* audioSystem = gm.GetSystems().audio_;
			auto* resourceMgr = gm.GetSystems().resourceManager_;
			if (!audioSystem || !resourceMgr) return;

			try {
				FMOD::Sound& sfx = resourceMgr->get<FMOD::Sound>("UI_Button_Cancel_01");
				audioSystem->play(sfx, 0.9f, false);
			}
			catch (...) {
				std::cerr << "[MainMenu] Missing SFX: UI_Button_Cancel_01\n";
			}
		}
	}

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

		// If the HOW TO PLAY overlay is open, we only care about its buttons
		if (howToPlayOpen) {
			if (buttonName == "HowToPlayLeftArrow") {
				PlayClickSFX();
				if (howToPlayPageIndex > 0) {
					--howToPlayPageIndex;
					UpdateHowToPlayTexture();
				}
			}
			else if (buttonName == "HowToPlayRightArrow") {
				PlayClickSFX();
				if (howToPlayPageIndex < kHowToPlayPageCount - 1) {
					++howToPlayPageIndex;
					UpdateHowToPlayTexture();
				}
				else {
					// Last page → close overlay and return to main menu
					CloseHowToPlayOverlay();
				}
			}

			// Ignore Play / Settings / Quit while overlay is open
			return;
		}
		
		// -------------------
	    // Normal CLICK SFX
	    // -------------------
		if (buttonName == "PlayButton") {
			PlayClickSFX();
			pendingPlay = true;
		}
		else if (buttonName == "HowToPlayButton") {
			PlayClickSFX();
			pendingHowToPlay = true;
		}
		else if (buttonName == "SettingsButton") {
			PlayClickSFX();
			pendingSettings = true;
		}

		// -------------------
	    // CANCEL SFX
	    // -------------------
		else if (buttonName == "SettingsCancelButton") {
			PlayCancelSFX();
			pendingCloseSettings = true;
		}
		else if (buttonName == "PauseResumeButton") {
			PlayClickSFX();
			pendingPauseClose = true;    // Resume = close pause menu
		}
		else if (buttonName == "QuitGameButton") {
			PlayCancelSFX();
			pendingQuit = true;
		}
	}

	void CreateHowToPlayButtonInCode() {
		EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
		SceneManager& sceneMgr = SceneManager::GetInstance();

		// 1) The button image (Button.png)
		Entity howToPlayButton = entityMgr.CreateEntity();
		entityMgr.GetProperties(howToPlayButton).name = "HowToPlayButton";

		// Position it between Play (0.2f) and Settings (0.0f)
		entityMgr.AddComponent(howToPlayButton, Position{ { 0.0f, 0.04f } });

		entityMgr.AddComponent(howToPlayButton, TextureComponent{
			"Button.png",          // use your generic button sprite
			{ 0.25f, 0.08f }       // same size as Play / Settings / Quit
			});

		entityMgr.AddComponent(howToPlayButton, Button{});   // makes it clickable
		entityMgr.AddComponent(howToPlayButton, GUITag{});   // this is UI
		entityMgr.AddComponent(howToPlayButton, ActiveTag{});// enabled
		sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), howToPlayButton);

		// 2) The text on top: "How To Play"
		Entity howToPlayButtonText = entityMgr.CreateEntity();
		entityMgr.GetProperties(howToPlayButtonText).name = "HowToPlayButtonText";

		// Same Y, slight X offset to visually center the text
		entityMgr.AddComponent(howToPlayButtonText, Position{ { -0.12f, 0.02f } });

		entityMgr.AddComponent(howToPlayButtonText, GUITag{});
		entityMgr.AddComponent(howToPlayButtonText, TextComponent{
			"How To Play",             // text
			"Amethysta-Regular.ttf",   // same font as others
			{ 1.0f, 1.0f, 1.0f, 1.0f },// white
			0.9f                       // font size
			});

		sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), howToPlayButtonText);
	}

	void CreateMainMenu() {
		EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
		SceneManager& sceneMgr = SceneManager::GetInstance();

		CreateHowToPlayButtonInCode();   // <--- here
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

	// Create HOW TO PLAY overlay: page 0 + left/right arrows
	void OpenHowToPlayOverlay() {
		if (howToPlayOpen) return;

		EntityManager entityMgr(*entityStorage_);
		SceneManager& sceneMgr = SceneManager::GetInstance();

		howToPlayPageIndex = 0;

		// --- Big panel with the current page image ---
		howToPlayPanel = entityMgr.CreateEntity();
		entityMgr.GetProperties(howToPlayPanel).name = "HowToPlayPanel";
		entityMgr.AddComponent(howToPlayPanel, Position{ { 0.0f, 0.0f } });
		entityMgr.AddComponent(howToPlayPanel, TextureComponent{
			"How To Play Image_0.png",    // first page
			{ 1.0f, 1.0f }                // full-screen-ish
			});
		entityMgr.AddComponent(howToPlayPanel, GUITag{});
		entityMgr.AddComponent(howToPlayPanel, ActiveTag{});
		sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), howToPlayPanel);

		// --- Left arrow button ---
		howToPlayLeftArrow = entityMgr.CreateEntity();
		entityMgr.GetProperties(howToPlayLeftArrow).name = "HowToPlayLeftArrow";
		entityMgr.AddComponent(howToPlayLeftArrow, Position{ { -0.8f, 0.0f } });
		entityMgr.AddComponent(howToPlayLeftArrow, TextureComponent{
			"Arrow Button Left.png",
			{ 0.07f, 0.07f }
			});
		entityMgr.AddComponent(howToPlayLeftArrow, Button{});
		entityMgr.AddComponent(howToPlayLeftArrow, GUITag{});
		entityMgr.AddComponent(howToPlayLeftArrow, ActiveTag{});
		sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), howToPlayLeftArrow);

		// --- Right arrow button ---
		howToPlayRightArrow = entityMgr.CreateEntity();
		entityMgr.GetProperties(howToPlayRightArrow).name = "HowToPlayRightArrow";
		entityMgr.AddComponent(howToPlayRightArrow, Position{ { 0.8f, 0.0f } });
		entityMgr.AddComponent(howToPlayRightArrow, TextureComponent{
			"Arrow Button Right.png",
			{ 0.07f, 0.07f }
			});
		entityMgr.AddComponent(howToPlayRightArrow, Button{});
		entityMgr.AddComponent(howToPlayRightArrow, GUITag{});
		entityMgr.AddComponent(howToPlayRightArrow, ActiveTag{});
		sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), howToPlayRightArrow);

		howToPlayOpen = true;
	}

	// Helper to safely destroy an entity
	static void DestroyEntityIfValid(EntityManager& entityMgr, SceneManager& sceneMgr, Entity& e) {
		if (e != 0 && entityMgr.ValidateEntity(e)) {   // ✅ check that it exists
			entityMgr.DestroyEntity(e);                // ✅ actually destroy it
		} 
		e = 0;                                         // mark handle as invalid
	}

	// Close HOW TO PLAY overlay
	void CloseHowToPlayOverlay() {
		if (!howToPlayOpen) return;

		EntityManager entityMgr(*entityStorage_);
		SceneManager& sceneMgr = SceneManager::GetInstance();

		DestroyEntityIfValid(entityMgr, sceneMgr, howToPlayPanel);
		DestroyEntityIfValid(entityMgr, sceneMgr, howToPlayLeftArrow);
		DestroyEntityIfValid(entityMgr, sceneMgr, howToPlayRightArrow);

		howToPlayOpen = false;
		howToPlayPageIndex = 0;
	}

	// Change the panel texture based on howToPlayPageIndex
	void UpdateHowToPlayTexture() {
		if (!howToPlayOpen || howToPlayPanel == 0) return;

		EntityManager entityMgr(*entityStorage_);
		TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(howToPlayPanel);

		switch (howToPlayPageIndex) {
		case 0: tex.textureKey = "How To Play Image_0.png"; break;
		case 1: tex.textureKey = "How To Play Image_1.png"; break;
		case 2: tex.textureKey = "How To Play Image_2.png"; break;
		default: tex.textureKey = "How To Play Image_0.png"; break;
		}
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
		
		// ─────────────────────────────────────────────
        // Main Menu BGM (looping)
        // ─────────────────────────────────────────────
		{
			GameManager& gm = GameManager::GetInstance();

			auto* audioSystem = gm.GetSystems().audio_;
			auto* resourceMgr = gm.GetSystems().resourceManager_;

			if (audioSystem && resourceMgr) {
				// 1) Load the sound. Use the actual extension you have (.wav / .ogg / .mp3).
				resourceMgr->loadSound(
					"MainMenu_BGM_1",                                      // sound key
					"Assets/Audio/MainMenu_BGM/MainMenu_BGM_1.wav",        // file path (relative)
					true,                                  // loop
					true                                   // stream
				);

				// 2) Play it (same signature as in TestScene.cpp)
				FMOD::Sound& bgm = resourceMgr->get<FMOD::Sound>("MainMenu_BGM_1");
				audioSystem->play(bgm, 0.6f, false);   // volume 0.6, not paused
			}
			else {
				std::cerr << "[MainMenu] AudioSystem or ResourceManager is null, "
					"cannot play MainMenu_BGM.\n";
			}
		}
		// ─────────────────────────────────────────────

		// Pre-load UI button SFX so helpers can play them quickly
		{
			GameManager& gm = GameManager::GetInstance();
			auto* audioSystem = gm.GetSystems().audio_;
			auto* resourceMgr = gm.GetSystems().resourceManager_;
			if (audioSystem && resourceMgr) {
				// normal click
				resourceMgr->loadSound(
					"UI_Button_Clicked_02",
					"Assets/Audio/Button UI/Button_Clicked_02.wav",
					false,   // no loop
					false    // no stream
				);
				// cancel / back
				resourceMgr->loadSound(
					"UI_Button_Cancel_01",
					"Assets/Audio/Button UI/Button_Cancel_01.wav",
					false,
					false
				);
			}
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

		// NEW: How To Play button
		if (pendingHowToPlay) {
			std::cout << "[MainMenu] HOW TO PLAY selected." << std::endl;
			OpenHowToPlayOverlay();   // <--- use the helper you implemented
			pendingHowToPlay = false;
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