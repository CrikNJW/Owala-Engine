#include "RestartMenu.h"
#include "EntityManager.h"
#include <iostream>

namespace Framework {
    void RestartMenu::Initialize() {
        // Load font for menu text
        try {
            menuFont_ = &resourceManager_->get<Font>("Amethysta-Regular.ttf");
        }
        catch (...) {
            std::cerr << "Warning: Menu font not loaded." << std::endl;
            menuFont_ = nullptr;
        }
    }

    void RestartMenu::Update() {
        // Check if player is dead
        if (IsPlayerDead()) {
            RenderRestartMenu();
            HandleMenuInput();
        }
    }

    bool RestartMenu::IsPlayerDead() {
        EntityManager entityMgr(*entityStorage_);

        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(PlayerTagType)) {
            if (!entityMgr.HasComponent(entity, ActiveTagType)) continue;

            if (entityMgr.HasComponent(entity, PlayerStateType)) {
                PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);
                if (state.isDead) {
                    if (entityMgr.HasComponent(entity, AnimationComponentType)) {
                        AnimationComponent& anim = entityMgr.GetComponent<AnimationComponent>(entity);
                        if (anim.currentState == AnimationState::DEATH && !anim.isPlaying) {
                            return true;
                        }
                    }
                    else {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    void RestartMenu::RenderRestartMenu() {
        graphics_->setAlphaBlend(true);

        graphics_->drawRectangle(
            { -1.0f, -1.0f },
            { 1.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 0.7f }
        );

        Vector2D menuMin = { -0.3f, -0.2f };
        Vector2D menuMax = { 0.3f, 0.2f };
        graphics_->drawRectangle(menuMin, menuMax, { 0.1f, 0.1f, 0.1f, 0.9f });

        float borderThickness = 0.005f;
        Color borderColor = { 0.8f, 0.0f, 0.0f, 1.0f }; // Red border

        // Top border
        graphics_->drawLine({ menuMin.x, menuMax.y }, { menuMax.x, menuMax.y }, borderColor);
        // Bottom border
        graphics_->drawLine({ menuMin.x, menuMin.y }, { menuMax.x, menuMin.y }, borderColor);
        // Left border
        graphics_->drawLine({ menuMin.x, menuMin.y }, { menuMin.x, menuMax.y }, borderColor);
        // Right border
        graphics_->drawLine({ menuMax.x, menuMin.y }, { menuMax.x, menuMax.y }, borderColor);

        if (menuFont_) {
            graphics_->drawText(
                *menuFont_,
                "YOU DIED",
                { -0.15f, 0.1f },
                1.2f,
                { 1.0f, 0.2f, 0.2f, 1.0f }
            );

            // "Restart Level" option
            Color restartColor = (selectedOption_ == 0) ? Color{ 1.0f, 1.0f, 0.0f, 1.0f }  // Yellow if selected
            : Color{ 0.8f, 0.8f, 0.8f, 1.0f }; // Gray if not selected

            graphics_->drawText(
                *menuFont_,
                selectedOption_ == 0 ? "> Restart Level" : "  Restart Level",
                { -0.2f, -0.05f },
                0.8f,
                restartColor
            );

            // "Quit to Menu" option
            Color quitColor = (selectedOption_ == 1) ? Color{ 1.0f, 1.0f, 0.0f, 1.0f }  // Yellow if selected
            : Color{ 0.8f, 0.8f, 0.8f, 1.0f }; // Gray if not selected

            graphics_->drawText(
                *menuFont_,
                selectedOption_ == 1 ? "> Quit to Menu" : "  Quit to Menu",
                { -0.2f, -0.15f },
                0.8f,
                quitColor
            );
        }
    }

    void RestartMenu::HandleMenuInput() {
        // Choose menu options with Up/Down arrows or W/S
        if (input_->KeyPressed(InputSystem::Key::Up) ||
            input_->KeyPressed(InputSystem::Key::W)) {
            selectedOption_ = (selectedOption_ - 1 + 2) % 2; // Cycle up
        }

        if (input_->KeyPressed(InputSystem::Key::Down) ||
            input_->KeyPressed(InputSystem::Key::S)) {
            selectedOption_ = (selectedOption_ + 1) % 2; // Cycle down
        }

        // Confirm selection with Enter or Space
        if (input_->KeyPressed(InputSystem::Key::Enter) ||
            input_->KeyPressed(InputSystem::Key::Space)) {
            if (selectedOption_ == 0) {
                RestartLevel();
            }
            else {
                std::cout << "Quit to menu requested" << std::endl;
                // TODO: Add quit to menu logic here
            }
        }

        // ESC directly quits to menu
        if (input_->KeyPressed(InputSystem::Key::Escape)) {
            std::cout << "Quit to menu requested" << std::endl;
        }
    }

    void RestartMenu::RestartLevel() {
        levelReset_->RunReset();
        selectedOption_ = 0; // Reset selection
    }

    void RestartMenu::Shutdown() {
        // Cleanup if needed
    }
}