/*****************************************************************//**
 * \file	CutsceneController.h
 *
 * \primary_author
 *      Jason Lou
 * \secondary_author

 *
 * \brief	Contains declarations for the Cutscene Controller.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once

#include "ScriptableEntity.h"

using namespace Framework;

/*****************************************************************//**
 * \class   CutsceneController
 * \brief   Script to manage cutscene slides and skipping logic.
 *********************************************************************/
class CutsceneController : public ScriptableEntity {
protected:
	/*****************************************************************//**
	 * \brief   Initializes the script.
	 *********************************************************************/
	void OnCreate() override;

	/*****************************************************************//**
	 * \brief   Updates cutscene logic and input every frame.
	 *********************************************************************/
	void OnUpdate() override;

	/*****************************************************************//**
	 * \brief   Cleans up upon destruction.
	 *********************************************************************/
	void OnDestroy() override;
private:
	/* Data Types */
	
	/*****************************************************************//**
	 * \struct  Slide
	 * \brief   Represents a single cutscene image and its duration.
	 *********************************************************************/
	struct Slide {
		std::string textureKey;
		float duration;
	};

	/* Variables */
	std::vector<Slide> slides;
	int currentSlide = 0;
	float timer = 0.f;
	bool isEnding = false;

	/* Variables for Skip handling. */
	float skipHoldTime = 0.f;
	bool wasSkipDown = false;
	bool skipTriggered = false;

	/* Methods */
	
	/*****************************************************************//**
	 * \brief   Updates the entity to show the current slide.
	 *********************************************************************/
	void ApplySlide();

	/*****************************************************************//**
	 * \brief   Moves to the next slide or ends the cutscene.
	 * \return  True if cutscene is finished, false otherwise.
	 *********************************************************************/
	bool NextSlide();

	/*****************************************************************//**
	 * \brief   Ends the cutscene and loads the next level.
	 *********************************************************************/
	void FinishCutscene();

	/*****************************************************************//**
	 * \brief   Ensures the image covers the entire screen.
	 *********************************************************************/
	void FitImageToScreen();

	/*****************************************************************//**
	 * \brief   Checks if the skip input is currently active.
	 * \return  True if skip is held down, false otherwise.
	 *********************************************************************/
	bool IsSkipHeldDown() const;
};
