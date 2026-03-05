/*****************************************************************//**
 * \file	CutsceneController.cpp
 *
 * \primary_author
 *      Jason Lou
 * \secondary_author

 *
 * \brief	Contains definitions for the Cutscene Controller.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "CutsceneController.h"
#include "ScriptRegistry.h"

 // Dependencies
#include "GameManager.h"
#include "EntityManager.h"
#include "SaveManager.h"
#include "InputSystem.h"
#include "Components.h"
#include "OwalaTime.h"
#include "Entity.h"
#include "WindowsSystem.h"
#include "SceneManager.h"

REGISTER_SCRIPT(CutsceneController, "CutsceneController")

void CutsceneController::CreateVideoPlayer(const std::string& videoPath) {
    EntityManager entityMgr(GameManager::GetInstance().GetECS());

    // 1. Create the entity
    Entity videoEntity = entityMgr.CreateEntity();
    entityMgr.AddComponent(videoEntity, Transform{});
    entityMgr.AddComponent(videoEntity, NativeScriptsComponent{});

    auto& scripts = entityMgr.GetComponent<NativeScriptsComponent>(videoEntity);

    // 2. Setup the script
    Framework::NativeScriptsComponent::NativeScript script;
    script.Bind<VideoPlayerScript>("VideoPlayerScript");
    script.enabled = true;
    scripts.scripts.push_back(script);

    // 3. CRITICAL: If GetScript returns null because it's not "Live" yet, 
    // we need to ensure the VideoPlayerScript knows where to look.
    // Instead of setting it here, let's make sure the path passed is correct.

    // If your engine allows Manual Instantiation, do it here. 
    // Otherwise, we must ensure VideoPlayerScript::OnCreate uses the correct path.
    videoPlayer = entityMgr.GetScript<VideoPlayerScript>(videoEntity);

    if (videoPlayer) {
        // We add "Assets/Sprites/Cutscenes/" prefix if it's missing
        std::string fullPath = videoPath;
        if (fullPath.find("/") == std::string::npos && fullPath.find("\\") == std::string::npos) {
            fullPath = "Assets/Sprites/Cutscenes/" + videoPath;
        }

        videoPlayer->videoPath = fullPath;
        videoPlayer->autoPlay = true;
        videoPlayer->loop = false;
    }

    SceneManager::GetInstance().AddEntityToScene(
        SceneManager::GetInstance().GetActiveScene().name,
        videoEntity
    );
}

void CutsceneController::ApplySlide() {
    if (currentSlide >= static_cast<int>(slides.size())) return;

    EntityManager entityMgr(GameManager::GetInstance().GetECS());
    Entity self = GetEntity();

    const Slide& slide = slides[currentSlide];

    if (slide.type == SlideType::Image) {
        // For images: set texture
        if (entityMgr.HasComponent(self, TextureComponentType)) {
            TextureComponent& texture = entityMgr.GetComponent<TextureComponent>(self);
            texture.textureKey = slide.source;
        }
        timer = 0.f;

        // Hide video player if it exists
        if (videoPlayer) {
            // Could hide by disabling entity, but we'll just let it run in background
            // or you could destroy it and recreate when needed
        }
    }
    else { // Video
        // Create or show video player
        CreateVideoPlayer(slide.source);

        // Hide the image texture
        if (entityMgr.HasComponent(self, TextureComponentType)) {
            TextureComponent& texture = entityMgr.GetComponent<TextureComponent>(self);
            texture.textureKey = ""; // Clear texture
        }
    }
}

bool CutsceneController::NextSlide() {
    currentSlide++;
    if (currentSlide >= static_cast<int>(slides.size())) {
        FinishCutscene();
        return true;
    }

    ApplySlide();
    return false;
}

void CutsceneController::FinishCutscene() {
    if (isEnding) return;
    isEnding = true;

    EntityManager entityMgr(GameManager::GetInstance().GetECS());

    // Search for SaveManager
    const std::vector<Entity> persistents = SceneManager::GetInstance().GetEntitiesInScene(SceneManager::GetDDOLSceneID());
    for (Entity entity : persistents) {
        if (!entityMgr.ValidateEntity(entity)) continue;
        if (SaveManager* save = entityMgr.GetScript<SaveManager>(entity)) {
            save->SetSkipIntro(true);
            break;
        }
    }

    GameManager::GetInstance().InitiateLevelLoad("Tutorial 1");
}

void CutsceneController::FitImageToScreen() {
    EntityManager entityMgr(GameManager::GetInstance().GetECS());

    Entity self = GetEntity();
    if (!entityMgr.HasComponent(self, TextureComponentType)) return;

    TextureComponent& texture = entityMgr.GetComponent<TextureComponent>(self);
    Transform& transform = entityMgr.GetComponent<Transform>(self);

    texture.HalfSize = { 1.f, 1.f };
    transform.position = { 0.f, 0.f };
}

bool CutsceneController::IsSkipHeldDown() const {
    InputSystem* input = GameManager::GetInstance().GetSystems().input_;
    if (!input) return false;

    return input->KeyDown(InputSystem::Key::Space) ||
        input->KeyDown(InputSystem::Key::F) ||
        input->KeyDown(InputSystem::Key::Escape);
}

void CutsceneController::OnCreate() {
    // Define your cutscene sequence - mix of images and videos!
    slides = {
        // First play a video
        {"Testvideo.mp4", 0.0f, SlideType::Video},
        // Then show images
        {"SB_CutScene_01.jpg", 3.5f, SlideType::Image},
        {"SB_CutScene_02.jpg", 3.5f, SlideType::Image},
        {"SB_CutScene_03.jpg", 3.5f, SlideType::Image},
        {"SB_CutScene_04.jpg", 3.5f, SlideType::Image},
        // Maybe another video at the end
    };

    currentSlide = 0;
    timer = 0.f;
    videoPlayer = nullptr;

    skipHoldTime = 0.f;
    wasSkipDown = skipTriggered = false;

    ApplySlide();
}

void CutsceneController::OnUpdate() {
    if (isEnding) return;

    // Handle video playback if current slide is video
    if (currentSlide < static_cast<int>(slides.size()) &&
        slides[currentSlide].type == SlideType::Video) {

        // Video automatically plays (autoPlay = true)
        // Check if video finished
        if (videoPlayer && videoPlayer->IsFinished()) {
            NextSlide();
            return;
        }
    }
    else {
        // For image slides, ensure they're fullscreen
        FitImageToScreen();
    }

    // Skip input handling (works for both images and videos)
    bool skipDown = IsSkipHeldDown();

    if (skipDown) {
        skipHoldTime += Time::DeltaTime();

        if (!skipTriggered && skipHoldTime >= 2.f) {
            skipTriggered = true;
            FinishCutscene();
            return;
        }
    }
    else {
        if (wasSkipDown && !skipTriggered) {
            if (skipHoldTime > 0.f && skipHoldTime < 2.f) {
                if (NextSlide()) return;
            }
        }

        skipHoldTime = 0.f;
        skipTriggered = false;
    }

    wasSkipDown = skipDown;

    // Auto-advance for image slides (videos are handled by their own timing)
    if (!skipDown && currentSlide < static_cast<int>(slides.size()) &&
        slides[currentSlide].type == SlideType::Image) {
        timer += Time::DeltaTime();
        if (timer >= slides[currentSlide].duration) {
            if (NextSlide()) return;
        }
    }
}

void CutsceneController::OnDestroy() {
    videoPlayer = nullptr;
    // Logic to be called before Entity is deleted
}