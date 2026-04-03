/*****************************************************************//**
 * \file   LoadingScreenController.h
 *
 * \primary_author
 *      Ng Jing Wen (80%)
 * \secondary_author
 *      Jordain Ng Ying Zhe (20%)
 *
 * \brief	Declares the LoadingScreenController script to manage loading 
 *          screen behavior.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include "ScriptableEntity.h"

namespace Framework {

    /*****************************************************************//**
     * \class   LoadingScreenController
     * \brief   Script to control the loading screen UI and scene transitions.
     *********************************************************************/
    class LoadingScreenController : public ScriptableEntity {
    protected:
        /*****************************************************************//**
         * \brief   Initializes the loading screen.
         *********************************************************************/
        void OnCreate() override;

        /*****************************************************************//**
         * \brief   Updates loading progress and transitions when complete.
         *********************************************************************/
        void OnUpdate() override;

        /*****************************************************************//**
         * \brief   Cleans up variables on destruction.
         *********************************************************************/
        void OnDestroy() override;
    };

    static float loadFadeAlpha = 0.0f;
    static bool  loadFadingOut = false;
    static bool  loadingDone = false;
    static std::string loadNextScene = "";

}