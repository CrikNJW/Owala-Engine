#include "MainMenu.h"

// Engine
#include "GameManager.h"

// Standard Library
#include <iostream>
#include <string>

// Pointers to engine systems
Framework::Font* menuFont_ = nullptr;

// Main menu entities
Entity menuBackground, menuFrame, menuTitle;
Entity playButton, playButtonText;
Entity settingsButton, settingsButtonText;
Entity quitButton, quitButtonText;

bool menuCreated = false;

// Actions queued from button clicks
bool pendingPlay = false;
bool pendingSettings = false;
bool pendingQuit = false;

namespace MainMenu {
	using namespace Framework;

	void HandleButtonClick(const ButtonClickedEvent& event) {
		EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
		std::string& buttonName = entityMgr.GetProperties(event.button).name;

		std::cout << "[MainMenu] Button clicked: " << buttonName << std::endl;

		if (buttonName == "PlayButton") {
			pendingPlay = true;
		}
		else if (buttonName == "SettingsButton") {
			pendingSettings = true;
		}
		else if (buttonName == "QuitGameButton") {
			pendingQuit = true;
		}
	}

	void CreateMainMenu() {
		EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
		SceneManager& sceneMgr = SceneManager::GetInstance();

		// ---------------------------
		// Background
		// ---------------------------
		menuBackground = entityMgr.CreateEntity();
		entityMgr.GetProperties(menuBackground).name = "MainMenuBackground";
		entityMgr.AddComponent(menuBackground, Position{ { 0.0f, 0.0f } });
		// NOTE:
		// RestartMenu uses TextureComponent{ "Button.png", {0.2f, 0.2f} };
		// so here we follow the same convention and only use the filename.
		// Make sure your ResourceManager has "Background.png" loaded and mapped.
		entityMgr.AddComponent(menuBackground, TextureComponent{ "Background.png", { 1.0f, 1.0f } });
		entityMgr.AddComponent(menuBackground, GUITag{});
		sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), menuBackground);

		// ---------------------------
		// Frame
		// ---------------------------
		menuFrame = entityMgr.CreateEntity();
		entityMgr.GetProperties(menuFrame).name = "MainMenuFrame";
		entityMgr.AddComponent(menuFrame, Position{ { 0.0f, 0.0f } });
		entityMgr.AddComponent(menuFrame, TextureComponent{ "Frame.png", { 0.8f, 0.8f } });
		entityMgr.AddComponent(menuFrame, GUITag{});
		sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), menuFrame);

		// ---------------------------
		// Title (image)
		// ---------------------------
		menuTitle = entityMgr.CreateEntity();
		entityMgr.GetProperties(menuTitle).name = "MainMenuTitle";
		entityMgr.AddComponent(menuTitle, Position{ { 0.0f, 0.6f } });
		entityMgr.AddComponent(menuTitle, TextureComponent{ "Title.png", { 0.6f, 0.2f } });
		entityMgr.AddComponent(menuTitle, GUITag{});
		sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), menuTitle);

		// ---------------------------
		// PLAY Button
		// ---------------------------
		playButton = entityMgr.CreateEntity();
		entityMgr.GetProperties(playButton).name = "PlayButton";
		entityMgr.AddComponent(playButton, Position{ { 0.0f, 0.2f } });
		entityMgr.AddComponent(playButton, TextureComponent{ "Button.png", { 0.25f, 0.08f } });
		entityMgr.AddComponent(playButton, Button{});
		entityMgr.AddComponent(playButton, GUITag{});
		entityMgr.AddComponent(playButton, ActiveTag{});
		sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), playButton);

		playButtonText = entityMgr.CreateEntity();
		entityMgr.GetProperties(playButtonText).name = "PlayButtonText";
		// Small left offset so the text appears centered visually on the button
		entityMgr.AddComponent(playButtonText, Position{ { -0.04f, 0.2f } });
		entityMgr.AddComponent(playButtonText, GUITag{});
		entityMgr.AddComponent(playButtonText, TextComponent{
			"Play", "Amethysta-Regular.ttf",
			{ 1.0f, 1.0f, 1.0f, 1.0f },
			0.9f
			});
		sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), playButtonText);

		// ---------------------------
		// SETTINGS Button
		// ---------------------------
		settingsButton = entityMgr.CreateEntity();
		entityMgr.GetProperties(settingsButton).name = "SettingsButton";
		entityMgr.AddComponent(settingsButton, Position{ { 0.0f, 0.0f } });
		entityMgr.AddComponent(settingsButton, TextureComponent{ "Button.png", { 0.25f, 0.08f } });
		entityMgr.AddComponent(settingsButton, Button{});
		entityMgr.AddComponent(settingsButton, GUITag{});
		entityMgr.AddComponent(settingsButton, ActiveTag{});
		sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), settingsButton);

		settingsButtonText = entityMgr.CreateEntity();
		entityMgr.GetProperties(settingsButtonText).name = "SettingsButtonText";
		entityMgr.AddComponent(settingsButtonText, Position{ { -0.09f, 0.0f } });
		entityMgr.AddComponent(settingsButtonText, GUITag{});
		entityMgr.AddComponent(settingsButtonText, TextComponent{
			"Settings", "Amethysta-Regular.ttf",
			{ 1.0f, 1.0f, 1.0f, 1.0f },
			0.9f
			});
		sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), settingsButtonText);

		// ---------------------------
		// QUIT GAME Button
		// ---------------------------
		quitButton = entityMgr.CreateEntity();
		entityMgr.GetProperties(quitButton).name = "QuitGameButton";
		entityMgr.AddComponent(quitButton, Position{ { 0.0f, -0.2f } });
		entityMgr.AddComponent(quitButton, TextureComponent{ "Button.png", { 0.25f, 0.08f } });
		entityMgr.AddComponent(quitButton, Button{});
		entityMgr.AddComponent(quitButton, GUITag{});
		entityMgr.AddComponent(quitButton, ActiveTag{});
		sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), quitButton);

		quitButtonText = entityMgr.CreateEntity();
		entityMgr.GetProperties(quitButtonText).name = "QuitGameButtonText";
		entityMgr.AddComponent(quitButtonText, Position{ { -0.11f, -0.2f } });
		entityMgr.AddComponent(quitButtonText, GUITag{});
		entityMgr.AddComponent(quitButtonText, TextComponent{
			"Quit Game", "Amethysta-Regular.ttf",
			{ 1.0f, 1.0f, 1.0f, 1.0f },
			0.9f
			});
		sceneMgr.AddEntityToScene(sceneMgr.GetActiveScene(), quitButtonText);
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

	void Initialize() {
		ResourceManager& resourceManager_ = *GameManager::GetInstance().GetSystems().resourceManager_;

		try {
			menuFont_ = &resourceManager_.get<Font>("Amethysta-Regular.ttf");
		}
		catch (...) {
			std::cerr << "Warning: MainMenu font not loaded." << std::endl;
			menuFont_ = nullptr;
		}

		CreateMainMenu();
		menuCreated = true;
		std::cout << "Main Menu Initialized!!!" << std::endl;
	}

	void Update() {
		std::cout << "Main Menu print" << std::endl;
	}

	void Free() {
		if (menuCreated) {
			CleanupMenu();
		}
	}
}