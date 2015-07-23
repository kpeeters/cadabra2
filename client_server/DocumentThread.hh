
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
   class ComputeThread;
   class GUIBase;

	/**
		\defgroup clientserver Client-Server 

      Base classes (independent of any GUI toolkit) to write Cadabra
      clients and servers.

		All clients use two threads. One is the main thread and runs
		most of the logic in the DocumentThread object. The other one is
		a thread which communicates with the server process; code for
		this thread is in the ComputeThread object.  One typically
		subclasses the former into a full-fledged object driving the
		graphical user interface.
		
		All functionality that deals with GUI updates is isolated in
		GUIBase. A client should derive from this abstract base class
		and implement the methods there. Clients should also derive from
		DocumentThread, but there are no abstract virtual members
		to implemented from this base class.
		
		All actual updates to the document stored in the
		DocumentThread::dtree member are made on the GUI thread as
		well. The compute thread merely puts requests to change the
		document onto a stack, and then requests that the GUI updates
		the document.
	*/

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

			void         build_visual_representation();
			
			/// All changes to the document should be made by submitting
			/// ActionBase derived objects to the 'queue_action' function,
			/// so that an undo stack can be kept. They are then processed
			/// by calling the 'process_action_queue' method (only
			/// available from this thread). Never directly modify the
			/// 'doc' Dtree object.
			
			void queue_action(std::shared_ptr<ActionBase>);


			/// Blah
			std::mutex   dtree_mutex;
			const DTree& dtree();
         void         new_document();


         friend ActionAddCell;
			friend ActionPositionCursor;
			friend ActionRemoveCell;
         // FIXME: add other actions.
	
		protected:

         GUIBase       *gui;
         ComputeThread *compute;
			DTree          doc;
			
         /// The action undo/redo/todo stacks and logic to execute
			/// them. These stacks can be accessed from both the
			/// DocumentThread as well as the ComputeThread, so they need
			/// a mutex to access them.

         std::mutex                                       stack_mutex;
			typedef std::stack<std::shared_ptr<ActionBase> > ActionStack;
			ActionStack                                      undo_stack, redo_stack;
			std::queue<std::shared_ptr<ActionBase> >         pending_actions;			

			/// Process the action queue. It is allowed to call queue_action() above
			/// while this is running. So a running action can add more actions.

         void                                             process_action_queue();
	};
	
}
