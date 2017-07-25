
#pragma once

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
	class ActionInsertText;
	class ActionEraseText;
   class ComputeThread;
   class GUIBase;

	/// \ingroup clientserver
	///
   /// A base class with all the logic to manipulate a Cadabra
   /// notebook document. Relies on the various objects derived from
   /// ActionBase in order to get actual work done. All methods here
   /// will always run on the GUI thread.
   ///
	/// In order to implement a GUI, derive from both DocumentThread
	/// and GUIBase and then implement the virtual functions of the
	/// latter (those implement basic insertion/removal of notebook
	/// cells; the logic to figure out which ones and to implement the
	/// undo/redo stack is all in the GUI-agnostic DocumentThread).


   class DocumentThread {
		public:
         DocumentThread(GUIBase *);

			/// It is not possible to copy-construct a DocumentThread as
			/// it holds on to resources which are not easily copied
			/// (such as GUI elements).

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

			/// One undo step.
			void undo();

         friend ActionAddCell;
			friend ActionPositionCursor;
			friend ActionRemoveCell;
			friend ActionSplitCell;
			friend ActionSetRunStatus;
			friend ActionInsertText;
			friend ActionEraseText;
	
			/// Determine if a user has been registered with the Cadabra
			/// log server. 

			bool is_registered() const;

			/// Set user details which will be sent to the Cadabra log
			/// server.

			void set_user_details(const std::string& name, const std::string& email, const std::string& affiliation);
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
			bool                                             disable_stacks;

			/// Process the action queue. It is allowed to call queue_action() above
			/// while this is running. So a running action can add more actions.

         void                                             process_action_queue();


			/// Configuration options read from ~/.config/cadabra.conf.
			bool  registered;

			/// Help system 
			enum class help_t { algorithm, property, latex, none };
			bool help_type_and_topic(const std::string& before, const std::string& after,
											 help_t& help_type, std::string& help_topic) const;

	};
	
}
