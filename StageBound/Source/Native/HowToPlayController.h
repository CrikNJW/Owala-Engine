/*****************************************************************//**
 * \file   HowToPlayController.h
 *
 * \primary_author
 *      Yar Xuan
 * \secondary_author
 *
 *
 * \brief	Header File for How To Play scene controller.
 *          Toggles entities active boolean to show/hide entities
 *          according to the current page.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once

#include "ScriptableEntity.h"

using namespace Framework;

/*****************************************************************//**
 * \class   HowToPlayController
 * \brief   Script to control the pages in the How To Play menu.
 *********************************************************************/
class HowToPlayController : public ScriptableEntity {
protected:
	/*****************************************************************//**
	 * \brief   Initializes the controller and caches viewable entities.
	 *********************************************************************/
	void OnCreate() override;

	/*****************************************************************//**
	 * \brief   Updates page visibility and button states every frame.
	 *********************************************************************/
	void OnUpdate() override;

	/*****************************************************************//**
	 * \brief   Cleans up when the entity is destroyed.
	 *********************************************************************/
	void OnDestroy() override;

private:
	int currentPage = 0;                                 //!< Current visible page index.
	int maxPages = 3;                                    //!< Total number of instruction pages.
	std::vector<std::vector<Entity>> pageCache_;         //!< Cached lists of entities for each page.
	Entity prevBtn, nextBtn;                             //!< Cached references to the navigation buttons.
};
