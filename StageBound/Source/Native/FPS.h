/*****************************************************************//**
 * \file   FPS.h
 *
 * \primary_author
 *      Jason Lou
 * \secondary_author
 *
 *
 * \brief	Header file for the FPS custom Native Script. When
 *			attached to an entity with a text component, will
 *			cause it to display current game FPS live.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once

#include "ScriptableEntity.h"

using namespace Framework;

/*****************************************************************//**
 * \class   FPS
 * \brief   Script that calculates and displays the current frames per second.
 *********************************************************************/
class FPS : public ScriptableEntity {
private:
	bool shouldHide = true;
	float accum_ = 0.f;
	int frames_ = 0;
protected:
	/*****************************************************************//**
	 * \brief   Initializes the FPS counters.
	 *********************************************************************/
	void OnCreate() override;

	/*****************************************************************//**
	 * \brief   Updates the FPS calculation and text display every frame.
	 *********************************************************************/
	void OnUpdate() override;

	/*****************************************************************//**
	 * \brief   Cleans up upon destruction.
	 *********************************************************************/
	void OnDestroy() override;
};
