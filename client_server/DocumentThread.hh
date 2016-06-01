
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
	class ActionPositionCursor;
	class ActionRemoveCell;
	class ActionSetRunStatus;
	class ActionSplitCell;
   class ComputeThread;
   class GUIBase;

	/// \ingroup clientserver
	///
	/// Base class which manipulates the document tree.

   class DocumentThread {
		public:
         DocumentThread(GUIBase *);
         DocumentThread(const DocumentThread&)=delete;

         /// Let the notebook know about the ComputeThread so that it
         /// can send cells for evaluation. Notebook does NOT own this
         /// pointer.

         void set_compute_thread(ComputeThread *);

			/// Ensure that the gui has an up-to-date representation of the
			/// dtree. Traverses the entire tree so is expensive to run and
			/// should only be used when loading new documents or creating
			/// a new canvas view.

			void build_visual_representation();
			
			/// All changes to the document should be made by submitting
			/// ActionBase derived objects to the 'queue_action' function,
			/// so that an undo stack can be kept. They are then processed
			/// by calling the 'process_action_queue' method (only
			/// available from this thread). 
			
			void queue_action(std::shared_ptr<ActionBase>);

			/// Setup an empty new document with a single Python input cell.

         void new_document();

			/// Load a new notebook from a JSON string. Should only be called
			/// from the GUI thread. Will cancel any pending operations on the
			/// existing notebook (if present) first.

			void load_from_string(const std::string&);

			/// Action objects are allowed to modify the DTree document doc,
			/// since they essentially contain code which is part of the 
			/// DocumentThread object.

         friend ActionAddCell;
			friend ActionPositionCursor;
			friend ActionRemoveCell;
			friend ActionSplitCell;
			friend ActionSetRunStatus;
         // FIXME: add other actions.
	
			bool is_registered() const;
			void set_user_details(const std::string&, const std::string&, const std::string&);
		protected:
         GUIBase       *gui;
         ComputeThread *compute;

			/// The actual document tree. This object is only modified on
			/// the GUI thread, either directly by code in
			/// DocumentThread, or by code in the various objects derived
			/// from ActionBase. In particular, ComputeThread cannot
			/// access this tree directly.

			DTree          doc;
			
         /// The action undo/redo/todo stacks and logic to execute
			/// them. These stacks can be accessed from both the
			/// DocumentThread as well as the ComputeThread (the latter
			/// does it through the DocumentThread::queue_action method),
			/// so they need a mutex to access them.

         std::mutex                                       stack_mutex;
			typedef std::stack<std::shared_ptr<ActionBase> > ActionStack;
			ActionStack                                      undo_stack, redo_stack;
			std::queue<std::shared_ptr<ActionBase> >         pending_actions;			

			/// Process the action queue. It is allowed to call queue_action() above
			/// while this is running. So a running action can add more actions.

         void                                             process_action_queue();


			/// Configuration options read from ~/.config/cadabra.conf.
			bool  registered;
	};
	
}
