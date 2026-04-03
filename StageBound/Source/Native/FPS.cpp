/*****************************************************************//**
 * \file   FPS.h
 *
 * \primary_author
 *      Jason Lou
 * \secondary_author
 *
 *
 * \brief	Source file for the FPS custom Native Script. When
 *			attached to an entity with a text component, will
 *			cause it to display current game FPS live.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "pch.h"
#include "FPS.h"
#include "ScriptRegistry.h"
#include "GameManager.h"

REGISTER_SCRIPT(FPS, "FPS")

/*****************************************************************//**
 * \brief   Initialize accumulator and frame counter.
 *********************************************************************/
void FPS::OnCreate() {
	accum_ = 0;
	frames_ = 0;
}

/*****************************************************************//**
 * \brief   Calculates and updates FPS text. Handles toggle input.
 *********************************************************************/
void FPS::OnUpdate() {
	EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
	InputSystem& inputSystem = *GameManager::GetInstance().GetSystems().input_;

	Entity self = GetEntity();

	// Toggle FPS visibility when F1 is pressed
	if (inputSystem.KeyPressed(InputSystem::Key::F1)) {
		shouldHide = !shouldHide;
		std::cout << (shouldHide ? "Hidden FPS Counter!" : "Showing FPS Counter") << std::endl;
	}

	// Only update if entity has a TextComponent
	if (entityMgr.HasComponent(self, ComponentType::TextComponentType)) {
		accum_ += Framework::Time::UnscaledDeltaTime();
		++frames_;

		const float updateInterval = 0.25f;
		
		// Update FPS text after the interval passes
		if (accum_ >= updateInterval) {
			float fps = frames_ / accum_;

			TextComponent& text = entityMgr.GetComponent<TextComponent>(self);
			text.isDirty = true;
			text.textToDraw = shouldHide ? "" : "FPS: " + std::to_string(static_cast<int>(fps+ 0.5f));
			
			// Reset counters
			accum_ = 0.f;
			frames_ = 0;
		}
	}
}

/*****************************************************************//**
 * \brief   Cleanup before entity is destroyed.
 *********************************************************************/
void FPS::OnDestroy() {
	// Logic to be called before Entity is deleted
}
