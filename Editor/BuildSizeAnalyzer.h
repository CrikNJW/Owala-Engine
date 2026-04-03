/*****************************************************************//**
* \file   BuildSizeAnalyzer.h
*
* \primary_author
*       Yar Xuan
*
* \brief    Header file for the BuildSizeAnalyzer in the Editor module.
*			The BuildSizeAnalyzer is an editor tool that scans project directories, displays asset sizes,
*			and allows the user to export selected assets to a destination folder.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <cstdint>

namespace Editor {

	struct BuildAssetItem {
		std::string relativePath; // Path relative to the source directory
		std::string filename;     // Name of the file
		uintmax_t size = 0;       // File size in bytes
		bool selected = true;     // Whether this asset is checked for export
	};

	class BuildSizeAnalyzer {
	public:
		BuildSizeAnalyzer() = default;
		~BuildSizeAnalyzer() = default;

		/*********************************************************************//**
		 * \brief   Renders the ImGui window and all its internal UI components.
		 *          Should be called every frame in the Editor's update loop.
		 *********************************************************************/
		void ShowUI();

		// --- Getters and Setters for Window State ---

		/*********************************************************************//**
		 * \brief   Checks if the BuildSizeAnalyzer window is currently open.
		 * 
		 * \return  True if the window is open, false otherwise.
		 *********************************************************************/
		bool IsOpen() const { return showBuildSizeAnalyzer_; }

		/*********************************************************************//**
		 * \brief   Sets the open state of the BuildSizeAnalyzer window.
		 * 
		 * \param   open
		 *********************************************************************/
		void SetOpen(bool open) { showBuildSizeAnalyzer_ = open; }

		/*********************************************************************//**
		 * \brief   Toggles the open state of the BuildSizeAnalyzer window.
		 *********************************************************************/
		void ToggleOpen() { showBuildSizeAnalyzer_ = !showBuildSizeAnalyzer_; }

	private:
		bool showBuildSizeAnalyzer_ = false;
		bool buildAnalyzerInitialized_ = false;

		std::vector<BuildAssetItem> buildAssets_;  // List of all discovered assets
		uintmax_t totalSelectedBuildSize_ = 0;     // Cached total size of all selected assets (in bytes)
		char exportDestinationBuffer_[512] = "";   // ImGui text buffer for the export destination path

		// --- Internal Helpers ---

		/*********************************************************************//**
		 * \brief   Scans the source directory for files and updates the 
		 *          buildAssets_ list.
		 *********************************************************************/
		void RefreshBuildAssets();

		/*********************************************************************//**
		 * \brief   Recalculates the totalSelectedBuildSize_ by summing the 
		 *          sizes of checked items in the asset list.
		 *********************************************************************/
		void CalculateTotalSelectedBuildSize();

#ifdef _WIN32
		/*********************************************************************//**
		 * \brief   Native Windows helper to open the standard folder picker dialog.
		 * 
		 * \return  The selected folder path as a string, or empty if cancelled.
		 *********************************************************************/
		std::string OpenFolderBrowser();
#endif

		/*********************************************************************//**
		 * \brief   Copies all selected assets to the destination folder 
		 *          specified in exportDestinationBuffer_.
		 *********************************************************************/
		void ExportSelectedBuildAssets();

		/*********************************************************************//**
		 * \brief   Converts raw bytes into a human-readable format.
		 * 
		 * \param   bytes
		 * \return  A string formatted with the appropriate suffix (B, KB, MB, GB).
		 *********************************************************************/
		std::string FormatBytes(uintmax_t bytes) const;
	};

}