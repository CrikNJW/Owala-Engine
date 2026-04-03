/*****************************************************************//**
* \file   History.h
*
* \primary_author
*       Jason Lou
* \secondary_author
*
*
* \brief    This file declares the HistoryManager class and other structures associated 
*			to the History (Undo/Redo System). History is a Editor-Only
*			system. The History system stores ECS changes in the Editor to the
*			Undo stack.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
// Entity
#include "Entity.h"
#include "EntityStorage.h"
// Math
#include "Vector2D.h"

// Standard Library
#include <string>
#include <vector>

namespace Editor{
	
	/*****************************************************************//**
	 * \enum    OpType
	 * \brief   Defines the types of trackable operations for the undo/redo system.
	 *********************************************************************/
	enum class OpType : uint8_t {
		None = 0,
		Create,
		Delete,
		ModifyComponent,
	};

	/*****************************************************************//**
	 * \enum    PayloadMode
	 * \brief   Specifies whether a recorded payload represents an entire
	 *          entity structure or a single component.
	 *********************************************************************/
	enum class PayloadMode : uint8_t {
		Component,
		Entity
	};

	/*****************************************************************//**
	 * \struct  Memento
	 * \brief   Stores the serialized JSON states of an object before and
	 *          after an operation, enabling bidirectional state restoration.
	 *********************************************************************/
	struct Memento {
		std::string beforeJSON;
		std::string afterJSON;
	};

	/*****************************************************************//**
	 * \struct  OpData
	 * \brief   Holds the core payload data required to restore or reapply
	 *          a recorded editor operation.
	 *********************************************************************/
	struct OpData {
		Entity entity = 0;
		PayloadMode mode = PayloadMode::Component;
		Framework::ComponentType componentType = Framework::ComponentType::MAX_COMPONENTS;
		Memento memento;
	};

	/*****************************************************************//**
	 * \struct  Operation
	 * \brief   Encapsulates a full trackable editor action, including descriptive
	 *          metadata and underlying manipulation data. Supports grouping.
	 *********************************************************************/
	struct Operation {
		OpType opType = OpType::None;
		std::string description;
		int groupID = 0;

		OpData data;
	};

	/*****************************************************************//**
	 * \class   HistoryManager
	 * \brief   Manages the undo and redo queues, providing APIs to record
	 *          operations (single or grouped) and apply state reversals.
	 *********************************************************************/
	class HistoryManager {
	private:
		/* Preservation */
		std::vector<Operation> undoStack; // Current operations that can be undone.
		std::vector<Operation> redoStack; // Current operations that can be redone.
		int maxHistorySteps{ 30 }; // Maximum stack size of the Undo Stack.

		// Might convert grouping system to vector of vector of operations in the future for better readability, see how - Jason.
		/* Grouping States */
		int nextGroupID = 1;
		int activeGroupID = 0;
		int groupDepth = 0;

		/* Internal Helpers */

		/*****************************************************************//**
		 * \brief   Prunes the Undo Stack to the limit, starting from the oldest operation in the Undo Stack.
		 *********************************************************************/
		void PruneHistoryToLimit();

		/*****************************************************************//**
		 * \brief   The actual application of the Undo/Redo operation.
		 *
		 * \param   op
		 * \param   isUndo
		 *********************************************************************/
		void Apply(const Operation& op, bool isUndo);

		/*****************************************************************//**
		 * \brief   Restores an entity to the state recorded in the JSON snapshot.
		 *
		 * \param   entity
		 * \param   json
		 *********************************************************************/
		void RestoreEntityFromSnapshot(Entity entity, const std::string& json);

		/*****************************************************************//**
		 * \brief   Restores a component to the state recorded in the JSON snapshot.
		 *
		 * \param   entity
		 * \param   type
		 * \param   json
		 *********************************************************************/
		void ApplyComponentSnapshot(Entity entity, Framework::ComponentType type, const std::string& json);

		/*****************************************************************//**
		 * \brief   Helper to handle LoadedTag.
		 *
		 * \param   entity
		 *********************************************************************/
		void SyncSceneLoadedState(Entity entity);
		
	public:
		HistoryManager() = default;

		/* Config */
		
		/*****************************************************************//**
		 * \brief   Sets the max limit of the Undo Stack. The Stack will be
		 *          pruned to fit the new limit if it already exceeds it.
		 *
		 * \param   limit
		 *********************************************************************/
		void SetUndoLimit(int limit);

		/* Accessors */
		
		/*****************************************************************//**
		 * \brief   Retrieves the description of the latest operation in the Undo Stack.
		 * \return  A string literal description of the action.
		 *********************************************************************/
		std::string GetLatestUndoDescription() const;
		
		/*****************************************************************//**
		 * \brief   Retrieves the description of the latest operation in the Redo Stack.
		 * \return  A string literal description of the action.
		 *********************************************************************/
		std::string GetLatestRedoDescription() const;
		
		/*****************************************************************//**
		 * \brief   Retrieves the descriptions of all operations in the Undo Stack.
		 * \return  Array of string descriptions mapped sequentially.
		 *********************************************************************/
		std::vector<std::string> GetUndoHistoryDescriptions() const;
		
		/*****************************************************************//**
		 * \brief   Retrieves the descriptions of all operations in the Redo Stack.
		 * \return  Array of string descriptions mapped sequentially.
		 *********************************************************************/
		std::vector<std::string> GetRedoHistoryDescriptions() const;
		
		/*****************************************************************//**
		 * \brief   Gets the current history stack limit (Readonly).
		 * \return  Current history stack limit.
		 *********************************************************************/
		const int GetUndoLimit() const { return maxHistorySteps; };

		/* APIs */
		
		/*****************************************************************//**
		 * \brief   Discards memory tracks formatting structure clearing history caches.
		 *********************************************************************/
		void ClearHistory();
		
		/*****************************************************************//**
		 * \brief   Function for recording Entity-Level operations.
		 *
		 * \param   opType
		 * \param   entity
		 * \param   description
		 *********************************************************************/
		void Record(OpType opType, Entity entity, const std::string& description);
		
		/*****************************************************************//**
		 * \brief   Dedicated Function for recording operations relating to Component Modifications.
		 *          Add/Remove/Modify Components should use this.
		 *
		 * \param   entity
		 * \param   description
		 * \param   beforeJSON
		 * \param   afterJSON
		 * \param   compType
		 *********************************************************************/
		void RecordModifyComponent(Entity entity, const std::string& description, const std::string& beforeJSON, const std::string& afterJSON, Framework::ComponentType compType = Framework::ComponentType::MAX_COMPONENTS);
		
		/*****************************************************************//**
		 * \brief  Pushes an operation to the Undo Stack. Do not use this unless you know what you are doing.
		 *
		 * \param   op 
		 *********************************************************************/
		void Push(Operation& op);
		
		/*****************************************************************//**
		 * \brief   Returns true if there is at least 1 operation in the Undo Stack.
		 *********************************************************************/
		bool CanUndo() const;
		
		/*****************************************************************//**
		 * \brief   Returns true if there is at least 1 operation in the Redo Stack.
		 *********************************************************************/
		bool CanRedo() const;
		
		/*****************************************************************//**
		 * \brief   Perform an Undo operation.
		 *********************************************************************/
		void Undo();
		
		/*****************************************************************//**
		 * \brief	Perform an Redo operation.
		 *********************************************************************/
		void Redo();

		/* Grouping APIs */
		
		/*****************************************************************//**
		 * \brief   Begins appending recorded operations following this call into a group.
		 *********************************************************************/
		void BeginGroup();
		
		/*****************************************************************//**
		 * \brief   Combines all operations mapped tracking modifications formatted after explicitly
					matched call defining group initialization logically terminating tracking sequences 
					functionally cleanly sequentially evaluating changes mapped formats effectively cleanly.
		 *********************************************************************/
		void EndGroup();
		
		/*****************************************************************//**
		 * \brief   Returns true if operations are currently being recorded as a grouped operation.
		 *********************************************************************/
		bool IsGroupActive() const noexcept;
	};
}