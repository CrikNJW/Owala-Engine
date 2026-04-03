/*****************************************************************//**
 * \file   ContinueButtonScript.h
 *
 * \primary_author
 *      Yar Xuan
 * \secondary_author
 *
 *
 * \brief	Header file for continue button logic to switch to next level scene.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once

#include "ScriptableEntity.h"

using namespace Framework;

/*****************************************************************//**
 * \class   ContinueButtonScript
 * \brief   Script that handles the continue button to load the next level.
 *********************************************************************/
class ContinueButtonScript : public ScriptableEntity {
protected:
	/*****************************************************************//**
	 * \brief   Called once when the script is created.
	 *********************************************************************/
	void OnCreate() override;

	/*****************************************************************//**
	 * \brief   Called every frame to update the button logic.
	 *********************************************************************/
	void OnUpdate() override;

	/*****************************************************************//**
	 * \brief   Called before the entity gets destroyed.
	 *********************************************************************/
	void OnDestroy() override;
private:
	Entity self_; //!< ID of the entity this script is attached to.
	bool initiatedNextLevelLoad = false; //!< Ensures the next level is only loaded once.
};
