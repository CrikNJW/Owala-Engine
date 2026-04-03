/*****************************************************************//**
 * \file	SaveManager.cpp
 *
 * \primary_author
 *      Jason Lou
 * \secondary_author

 *
 * \brief	Contains definitions for the SaveManager script
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "SaveManager.h"
#include "ScriptRegistry.h"

// Dependencies
#include "FileUtility.h"
#include "SceneManager.h"
#include "GameManager.h"


REGISTER_SCRIPT(SaveManager, "SaveManager")

/*****************************************************************//**
 * \brief   Checks if a given level index is unlocked.
 *********************************************************************/
bool SaveManager::IsUnlocked(int levelIndex) const {
	return levelIndex <= highestUnlockedLevel_;
}

/*****************************************************************//**
 * \brief   Unlocks a level and saves progress if it is a new highest level.
 *********************************************************************/
void SaveManager::Unlock(int levelIndex) {
	if (levelIndex > highestUnlockedLevel_) {
		highestUnlockedLevel_ = levelIndex;
		Save();
	}
}

/*****************************************************************//**
 * \brief   Directly sets the highest unlocked level and saves progress.
 *********************************************************************/
void SaveManager::SetUnlock(int levelIndex) {
	highestUnlockedLevel_ = levelIndex;
	if (highestUnlockedLevel_ <= 0) {
		highestUnlockedLevel_ = 1;
	}
	Save();
}

/*****************************************************************//**
 * \brief   Loads progress from the save file. Starts fresh if missing.
 *********************************************************************/
void SaveManager::Load() {
	rapidjson::Document doc;

	// Check if file exists
	if (!Framework::FileIOSystem::ReadFile(SAVE_FILEPATH, doc) || !doc.IsObject()) {
		highestUnlockedLevel_ = 1;
		skipIntro = false;
		Save();
		return;
	}

	highestUnlockedLevel_ = 1;
	skipIntro = false;

	if (doc.HasMember(KEY_HIGHEST_UNLOCKED) && doc[KEY_HIGHEST_UNLOCKED].IsInt()) {
		highestUnlockedLevel_ = doc[KEY_HIGHEST_UNLOCKED].GetInt();
		if (highestUnlockedLevel_ < 1) highestUnlockedLevel_ = 1;
	}

	if (doc.HasMember(KEY_SKIP_INTRO) && doc[KEY_SKIP_INTRO].IsBool()) {
		skipIntro = doc[KEY_SKIP_INTRO].GetBool();
	}
}

/*****************************************************************//**
 * \brief   Writes current progress to the save file.
 *********************************************************************/
void SaveManager::Save() const {
	rapidjson::Document doc;

	doc.SetObject();
	rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

	doc.AddMember(rapidjson::StringRef(KEY_HIGHEST_UNLOCKED), highestUnlockedLevel_, alloc);
	doc.AddMember(rapidjson::StringRef(KEY_SKIP_INTRO), skipIntro, alloc);

	Framework::FileIOSystem::WriteFileJson(SAVE_FILEPATH, doc);
}

/*****************************************************************//**
 * \brief   Initializes script, ensures persistence, and loads save data.
 *********************************************************************/
void SaveManager::OnCreate() {
	Framework::SceneManager& sceneMgr = Framework::SceneManager::GetInstance();
	
	// In case this isn't persistent.
	sceneMgr.SetEntityAsPersistent(GetEntity(), true);
	Load();
}

/*****************************************************************//**
 * \brief   Frame update logic.
 *********************************************************************/
void SaveManager::OnUpdate() {
	// Per-Frame Logic Here
}

/*****************************************************************//**
 * \brief   Cleanup before entity is destroyed.
 *********************************************************************/
void SaveManager::OnDestroy() {
	// Logic to be called before Entity is deleted
}