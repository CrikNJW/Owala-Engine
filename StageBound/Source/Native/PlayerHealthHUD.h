/*****************************************************************//**
 * \file	PlayerHealthHUD.h
 *
 * \primary_author
 *      Yar Xuan
 * \secondary_author

 *
 * \brief	Contains declarations for the PlayerHealthHUD script
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once

#include "ScriptableEntity.h"

using namespace Framework;

/*****************************************************************//**
 * \class   PlayerHealthHUD
 * \brief   Updates a HUD heart element when the player loses a life.
 *********************************************************************/
class PlayerHealthHUD : public ScriptableEntity {
protected:
	/*****************************************************************//**
	 * \brief   Finds the entity ID.
	 *********************************************************************/
	void OnCreate() override;

	/*****************************************************************//**
	 * \brief   Tracks player lives and updates texture/VFX if lost.
	 *********************************************************************/
	void OnUpdate() override;

	/*****************************************************************//**
	 * \brief   Cleans up before destruction.
	 *********************************************************************/
	void OnDestroy() override;

private:
	Entity self_, player_;
	int lifeNum_;
	bool lifeLost_;
};
