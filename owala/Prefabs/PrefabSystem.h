/*****************************************************************//**
* \file   PrefabSystem.h
*
* \primary_author
*      Sitara Anil
* \secondary_author
*     
* \brief
*      System for loading and managing entity prefabs from JSON files.
*      Supports creating entities from prefab templates with specialized
*      platform types (basic, trap, and scaling platforms). Provides
*      querying and filtering of prefabs by component type.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "Entity.h"
#include "EntityStorage.h"
#include "rapidjson/document.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>

namespace Framework {

    /*****************************************************************//**
     * \struct  ComponentData
     * \brief   Stores raw JSON data and type identifier for a component.
     *********************************************************************/
    struct ComponentData {
        std::string type;
        rapidjson::Document data;

        ComponentData() : type(""), data() {}
        ComponentData(const std::string& t) : type(t), data() {}
    };

    /*****************************************************************//**
     * \struct  EntityPrefab
     * \brief   Stores a complete definition of a prefab.
     *********************************************************************/
    struct EntityPrefab {
        std::string name;
        std::vector<ComponentData> components;
        std::vector<std::string> tags;
        std::string sourceFile;
        std::string filePath;
        int layer = 0;                          // Collision layer ID
        std::string layer_collision_name = "";  // Changed from int to string
        int renderLayer = 0;                    // Render layer ID
        std::string layer_render_name = "";     // Changed from int to string
        int customRenderOrder = 0;              // Custom render order within layer

        EntityPrefab() : name(""), layer(0), layer_collision_name("Default"), renderLayer(0), layer_render_name(""), customRenderOrder(0), sourceFile("") { }
    };

    /*****************************************************************//**
     * \struct  PlatformPrefab
     * \brief   Stores properties relevant to a standard platform.
     *********************************************************************/
    struct PlatformPrefab {
        std::string textureID;
        float mass = 0.0f;
    };

    /*****************************************************************//**
     * \struct  TrapPlatformPrefab
     * \brief   Stores properties specifically for trap platforms.
     *********************************************************************/
    struct TrapPlatformPrefab {
        std::string textureID;
        float mass = 0.0f;
        float fallSpeed = 0.0f;
        float triggerDelay = 0.0f;
    };

    /*****************************************************************//**
     * \class   PrefabSystem
     * \brief   Manages loading, saving, and instantiation of entity prefabs.
     *********************************************************************/
    class PrefabSystem {
    private:
        std::unordered_map<std::string, EntityPrefab> prefabs_;
        std::string prefabFolderPath_;

        /*****************************************************************//**
         * \brief   Extracts component basic info from a JSON value.
         *********************************************************************/
        void ParseComponentData(const rapidjson::Value& compObj, ComponentData& outData);

        /*****************************************************************//**
         * \brief   Adds a deserialized component to the target entity.
         *********************************************************************/
        void DeserializeComponent(EntityStorage& storage, Entity entity, const ComponentData& compData);

        /*****************************************************************//**
         * \brief   Parses and loads a single prefab JSON file.
         *********************************************************************/
        bool LoadSinglePrefabFile(const std::string& filepath);

        /*****************************************************************//**
         * \brief   Filters and extracts platform specific properties.
         *********************************************************************/
        PlatformPrefab ExtractPlatformData(const EntityPrefab& prefab) const;

        /*****************************************************************//**
         * \brief   Filters and extracts trap platform properties.
         *********************************************************************/
        TrapPlatformPrefab ExtractTrapPlatformData(const EntityPrefab& prefab) const;

    public:
        /*****************************************************************//**
         * \brief   Loads all prefab JSON files from a specified folder.
         *********************************************************************/
        bool LoadPrefabsFromFolder(const std::string& folderPath);

        /*****************************************************************//**
         * \brief   Wrapper to load a specific file into the prefab system.
         *********************************************************************/
        bool LoadPrefabFile(const std::string& filepath);

        /*****************************************************************//**
         * \brief   Clears memory and re-reads all prefabs in the current directory.
         *********************************************************************/
        bool ReloadAllPrefabs();

        /*****************************************************************//**
         * \brief   Reads an array of prefabs stored inside a single monolithic file.
         *********************************************************************/
        bool LoadPrefabsFromFile(const std::string& filename);

        /*****************************************************************//**
         * \brief   Serializes an active entity and saves it as a new prefab.
         *********************************************************************/
        bool SaveEntityAsPrefab(EntityStorage& storage, Entity entity, const std::string& prefabName, const std::string& customFolder = "Custom");

        /*****************************************************************//**
         * \brief   Deletes the prefab from memory and its respective disk file.
         *********************************************************************/
        bool DeletePrefab(const std::string& prefabName);

        /*****************************************************************//**
         * \brief   Spawns a new entity based on the given prefab name.
         *********************************************************************/
        Entity CreateEntityFromPrefab(EntityStorage& storage, const std::string& prefabName, const Vector2D& position);

        /*****************************************************************//**
         * \brief   Checks if a prefab configuration exists in memory.
         *********************************************************************/
        bool HasPrefab(const std::string& name) const;

        /*****************************************************************//**
         * \brief   Gets a read-only pointer to a loaded prefab configuration.
         *********************************************************************/
        const EntityPrefab* GetPrefab(const std::string& name) const;

        /*****************************************************************//**
         * \brief   Returns the entire map of loaded prefabs.
         *********************************************************************/
        const std::unordered_map<std::string, EntityPrefab>& GetAllPrefabs() const {
            return prefabs_;
        }

        /*****************************************************************//**
         * \brief   Gets the current folder where prefabs are loaded from.
         *********************************************************************/
        const std::string& GetPrefabFolderPath() const { return prefabFolderPath_; }

        /*****************************************************************//**
         * \brief   Returns maps of platform specific configs parsed from prefabs.
         *********************************************************************/
        std::unordered_map<std::string, PlatformPrefab> GetPlatformPrefabs() const;
        std::unordered_map<std::string, TrapPlatformPrefab> GetTrapPlatformPrefabs() const;

        /*****************************************************************//**
         * \brief   Retrieves specific platform configs by their name.
         *********************************************************************/
        const PlatformPrefab* GetPlatformPrefab(const std::string& name) const;
        const TrapPlatformPrefab* GetTrapPlatformPrefab(const std::string& name) const;

        /*****************************************************************//**
         * \brief   Updates JSON data for a specific component within a prefab.
         *********************************************************************/
        bool UpdatePrefabComponentData(const std::string& prefabName, const std::string& componentType, const rapidjson::Value& newData);

        /*****************************************************************//**
         * \brief   Retrieves a mutable pointer for editing a loaded prefab directly.
         *********************************************************************/
        EntityPrefab* GetMutablePrefab(const std::string& name);

        /*****************************************************************//**
         * \brief   Gets a list of all tracked prefab files inside the root directory.
         *********************************************************************/
        std::vector<std::string> GetPrefabFileList() const;

        /*****************************************************************//**
         * \brief   Saves an actively modified prefab back to disk.
         *********************************************************************/
        bool SavePrefabToFile(const std::string& prefabName);
    };

} // namespace Framework