/*****************************************************************//**
 * \file	SaveManager.h
 *
 * \primary_author
 *      Jason Lou
 * \secondary_author

 *
 * \brief	Contains declarations for the SaveManager script
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once

#include "ScriptableEntity.h"

using namespace Framework;

/*****************************************************************//**
 * \class   SaveManager
 * \brief   Script handling level progression and saving/loading data.
 *********************************************************************/
class SaveManager : public ScriptableEntity {
protected:
	/*****************************************************************//**
	 * \brief   Initializes the manager and loads save data.
	 *********************************************************************/
	void OnCreate() override;

	/*****************************************************************//**
	 * \brief   Updates script logic every frame.
	 *********************************************************************/
	void OnUpdate() override;

	/*****************************************************************//**
	 * \brief   Cleans up before destruction.
	 *********************************************************************/
	void OnDestroy() override;
private:
	int highestUnlockedLevel_ = 1; //!< The highest level the player has reached.
	bool skipIntro = false;        //!< Flag to skip the intro cutscene.

	static constexpr const char* SAVE_FILEPATH = "Data/save.json";
	static constexpr const char* KEY_HIGHEST_UNLOCKED = "highest_unlocked_level";
	static constexpr const char* KEY_SKIP_INTRO = "skip_intro";
public:
	/*****************************************************************//**
	 * \brief   Checks if a level is currently accessible.
	 *********************************************************************/
	bool IsUnlocked(int levelIndex) const;

	/*****************************************************************//**
	 * \brief   Unlocks a level and saves if it progresses the player.
	 *********************************************************************/
	void Unlock(int levelIndex);

	/*****************************************************************//**
	 * \brief   Forcefully sets the unlock progress and saves.
	 *********************************************************************/
	void SetUnlock(int levelIndex);

	/*****************************************************************//**
	 * \brief   Retrieves the highest unlocked level.
	 *********************************************************************/
	int GetHighestProgression() const { return highestUnlockedLevel_; }

	/*****************************************************************//**
	 * \brief   Checks if the game should skip the intro sequence.
	 *********************************************************************/
	bool GetSkipIntro() const { return skipIntro; }

	/*****************************************************************//**
	 * \brief   Sets the intro skip flag and immediately saves.
	 *********************************************************************/
	void SetSkipIntro(bool value) { skipIntro = value; Save(); }
private:
	/*****************************************************************//**
	 * \brief   Reads saved progression from the save file.
	 *********************************************************************/
	void Load();

	/*****************************************************************//**
	 * \brief   Writes current progression to the save file.
	 *********************************************************************/
	void Save() const;
};
