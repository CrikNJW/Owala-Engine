#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "InputSystem.h"
#include "LevelResetSystem.h"
#include "Graphics.h"
#include "ResourceManager.h"

namespace Framework {
    class RestartMenu : public ISystem {
    public:
        RestartMenu(EntityStorage& entityStorage, InputSystem& input,
            LevelResetSystem& levelReset, Graphics& graphics,
            ResourceManager& resourceManager)
            : entityStorage_(&entityStorage), input_(&input),
            levelReset_(&levelReset), graphics_(&graphics),
            resourceManager_(&resourceManager) {}

        void Initialize() override;
        void Update() override;
        void Shutdown() override;

    private:
        void RenderRestartMenu();
        void HandleMenuInput();
        void RestartLevel();
        bool IsPlayerDead();

        EntityStorage* entityStorage_ = nullptr;
        InputSystem* input_ = nullptr;
        LevelResetSystem* levelReset_ = nullptr;
        Graphics* graphics_ = nullptr;
        ResourceManager* resourceManager_ = nullptr;

        // Menu state
        int selectedOption_ = 0; // 0 = Restart, 1 = Quit
        Font* menuFont_ = nullptr;
    };
}