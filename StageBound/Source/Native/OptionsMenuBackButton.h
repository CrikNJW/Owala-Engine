/*****************************************************************//**
 * \file   OptionsMenuBackButton.h
 *
 * \primary_author
 *      Ng Jing Wen
 * \secondary_author
 *
 *
 * \brief	Declaration of the OptionsMenuBackButton script to handle
 *			back button functionality in the options menu.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once

#include "ScriptableEntity.h"
#include "GameManager.h"
using namespace Framework;

/*****************************************************************//**
 * \class   OptionsMenuBackButton
 * \brief   Script to close the options menu when the back button is clicked.
 *********************************************************************/
class OptionsMenuBackButton : public ScriptableEntity {
private:
	bool shouldCloseMenu_;
protected:
	/*****************************************************************//**
	 * \brief   Initializes the close flag.
	 *********************************************************************/
	void OnCreate() override;

	/*****************************************************************//**
	 * \brief   Checks for clicks and triggers menu closing.
	 *********************************************************************/
	void OnUpdate() override;

	/*****************************************************************//**
	 * \brief   Cleans up before destruction.
	 *********************************************************************/
	void OnDestroy() override;
};
