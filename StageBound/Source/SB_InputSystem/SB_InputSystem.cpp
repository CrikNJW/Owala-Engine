/*********************************************************************//**
 * \file   SB_InputSystem.cpp
 *
 * \primary_author
 *      Ng Jordain Ying Zhe (80%)
 * \secondary_author
 *      Yar Xuan (20%)
 *
 * \brief   StageBound specific input system. Contains all testing and debugging keyboard shortcuts.
 *
 * \copyright
 *   Copyright (C) 2025 DigiPen Institute of Technology.
 *   Reproduction or disclosure of this file or its contents without
 *   the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "pch.h"
#include "SB_InputSystem.h"
#include "InputSystem.h"
#include "GameManager.h"

using EngineInput = Framework::InputSystem;

namespace Framework {

    /*********************************************************************//**
    * \brief   Updates the StageBound specific input system.
    *********************************************************************/
    void SB_InputSystem::Update() {
        // moved all testing keybinds to Editor 
    }

} // namespace Framework