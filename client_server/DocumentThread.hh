
#pragma once

// A base class with all the logic that will run on the GUI thread.
// In order to implement a GUI, create a derived class and implement
// the pure virtual methods here.

#include <queue>
#include <mutex>
#include <stack>

#include "DataCell.hh"
#include "tree.hh"

namespace cadabra {

   class ActionBase;
   class ActionAddCell;

   class DocumentThread {
		public:
			
			// The document is a tree of DataCells. All changes to the
			// tree should be made by submitting ActionBase derived
			// objects to the 'perform' function, so that an undo stack
			// can be kept.
			
			std::mutex   dtree_mutex;
			const DTree& dtree();
			
			bool perform(std::shared_ptr<ActionBase>);
		
         friend ActionAddCell;
	
		protected:
			
			// The actual document, the actions that led to it, and mutexes for
			// locking.
			DTree doc;
			
			typedef std::stack<std::shared_ptr<ActionBase> > ActionStack;
			ActionStack undo_stack, redo_stack;
			
			// Execution logic.
			void execute_undo_stack_top();

			// Actions which still have to be performed and then put on
			// the undo_stack.
			std::queue<std::shared_ptr<ActionBase> > pending_actions;
			
	};
	
}
