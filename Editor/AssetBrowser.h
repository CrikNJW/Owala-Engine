/*****************************************************************//**
* \file   AssetBrowser.h
*
* \primary_author
*       Ng Jing Wen
* \secondary_author
*      
*
* \brief    Contains declarations and definitions of an AssetBrowser that are
*           used to navigate, view, and manage assets within a specified directory.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "ISystem.h"
#include <vector>
#include <string>
#include <filesystem>
#include "ResourceManager.h"
#include <algorithm>
#include <imgui.h>

namespace fs = std::filesystem;
using Texture = Framework::Texture;
using ResourceManager = Framework::ResourceManager;

struct AssetEntry
{
	fs::path path;		   // Full path to the asset
    std::string name;      // File/folder name
    std::string display;   // What is shown in UI
    bool isDirectory = false;
    Texture iconTextureID = 0;
};

class AssetBrowser {
public:
    /*********************************************************************//**
     * \brief   Starts the browser from the current working directory.
     *          Useful as a safe default before a project is explicitly loaded.
     * 
     * \param   rcm 
     *********************************************************************/
    AssetBrowser(ResourceManager& rcm) : resourceManager_(&rcm), rootPath_(fs::current_path()), currentPath_(fs::current_path()) {
        if (fs::exists(currentPath_))
            ScanDirectory(currentPath_);
    }

    /*********************************************************************//**
     * \brief   Sets the project root and prepares local/repo sync paths.
     * 
     * \param   rootPath 
     *********************************************************************/
    void Initialize(const fs::path& rootPath);

    /*********************************************************************//**
     * \brief   Rebuilds the current asset list from a directory on disk.
     * 
     * \param   directoryPath 
     *********************************************************************/
    void ScanDirectory(const fs::path& directoryPath);

    /*********************************************************************//**
     * \brief   Enters a folder or selects a file based on the target path.
     * 
     * \param   target 
     *********************************************************************/
    void NavigateTo(const fs::path& target);

    /*********************************************************************//**
     * \brief   Moves up one folder level, clamped so it does not browse 
     *          outside the intended root path.
     *********************************************************************/
    void NavigateUp();

    /*********************************************************************//**
     * \brief   Draws the left-hand folder tree recursively in ImGui.
     * 
     * \param   path 
     *********************************************************************/
    void ShowFolderTree(const fs::path& path);

    /*********************************************************************//**
     * \brief   Draws the Project Explorer and Asset Viewer windows.
     * 
     * \param   droppedFiles 
     *********************************************************************/
    void RenderUI(std::vector<fs::path>& droppedFiles);

    /*********************************************************************//**
     * \brief   Draws the right-hand Asset Editor panel in ImGui.
     * 
     * \param   droppedFiles 
     *********************************************************************/
    void RenderAssetEditor(std::vector<fs::path>& droppedFiles);

    /*********************************************************************//**
     * \brief   Queues files dropped from the OS/file explorer into the viewer.
     * 
     * \param   droppedFiles 
     *********************************************************************/
    void HandleDroppedFiles(const std::vector<fs::path>& droppedFiles);

    /*********************************************************************//**
     * \brief   Handles replacement mode drops, including same-extension checking.
     * 
     * \param   droppedFiles 
     *********************************************************************/
    void HandleReplaceFile(const std::vector<fs::path>& droppedFiles);

    /*********************************************************************//**
     * \brief   Gets the current list of asset entries in the viewed directory.
     * 
     * \return  A constant reference to an array of AssetEntry objects.
     *********************************************************************/
    const std::vector<AssetEntry>& GetCurrentAssets() const { return currentAssets_; }

    /*********************************************************************//**
     * \brief   Gets the absolute path of the currently viewed directory.
     * 
     * \return  A constant reference to the current filesystem path.
     *********************************************************************/
    const fs::path& GetCurrentPath() const { return currentPath_; }

private:
    /*********************************************************************//**
     * \brief   Loads and caches viewer icon textures once per session.
     *********************************************************************/
    void EnsureIconTexturesLoaded();

    /*********************************************************************//**
     * \brief   Initializes a batch upload state for dropped files, preparing 
     *          them for processing over multiple frames.
     * 
     * \param   droppedFiles 
     *********************************************************************/
    void StartUploadQueue(const std::vector<fs::path>& droppedFiles);

    /*********************************************************************//**
     * \brief   Processes one queued upload file item per frame.
     *********************************************************************/
    void ProcessUploadQueue();

    /*********************************************************************//**
     * \brief   Copies one dropped file to the default local path and 
     *          optionally to the repository mirror.
     * 
     * \param   sourcePath 
     * \return  True if the file was successfully copied, otherwise false.
     *********************************************************************/
    bool CopyDroppedFile(const fs::path& sourcePath);

    /*********************************************************************//**
     * \brief   Shortens a text label to fit a designated width by appending
     *          ellipses if the text exceeds the boundary.
     * 
     * \param   text 
     * \param   maxWidth 
     * \param   wasTruncated 
     * \return  The resulting string, truncated if necessary.
     *********************************************************************/
    std::string TruncateLabelToFit(const std::string& text, float maxWidth, bool& wasTruncated) const;

    // Browser navigation state
    fs::path rootPath_;
    fs::path currentPath_;
    fs::path projectRootPath_;
	fs::path selectedAsset_;
    std::vector<AssetEntry> currentAssets_;
	ResourceManager* resourceManager_ = nullptr;

	// Replace/delete UI state
    bool isReplaceModeActive_ = false;
    fs::path replacementFilePath_;
    bool showReplaceConfirmation_ = false;
    bool showDeleteConfirmation_ = false;

    // Local output paths (build/Debug) and source project mirror paths
    fs::path localAssetsPath_;
    fs::path localDataPath_;
    fs::path repoAssetsPath_;
    fs::path repoDataPath_;
    bool syncToRepo_ = false;

    // Cached icon textures used by non-image asset tiles
    bool iconsLoaded_ = false;
    Texture folderIconTexture_ = 0;
    Texture fileIconTexture_ = 0;
    Texture fontIconTexture_ = 0;
    Texture audioIconTexture_ = 0;

    // Upload overlay / popup state
    std::vector<fs::path> uploadQueue_;
    bool uploadInProgress_ = false;
    bool showUploadPopup_ = false;
    bool uploadHadErrors_ = false;
    size_t uploadTotalFiles_ = 0;
    size_t uploadProcessedFiles_ = 0;
    double uploadPopupDismissAt_ = 0.0;
    std::string uploadCurrentFile_;

    // Drag payload path used by ImGui drag-drop source.
    std::string currentDraggedPath_;
};
