
#pragma once

// All modifications to the document are done by calling 'perform' with an 
// action object. This enables us to implement an undo stack. This method
// will take care of making the actual change to the DTree document, and
// call back on the 'change' methods above to inform the derived class
// that a change has been made. 

#include "DataCell.hh"
#include "DocumentThread.hh"

#include <memory>

namespace cadabra {

	class DocumentThread;
	class GUIBase;

	/// \ingroup clientserver
	///
	/// All actions derive from the ActionBase object, which defines
	/// the interface they need to implement. These objects are used to
	/// pass (user) action instructions around.  They can be stored in
	/// undo/redo stacks. All actions run on the GUI thread. The
	/// update_gui members typically call members of the GUIBase class.
	/// Action objects are allowed to modify the DTree document doc,
	/// since they essentially contain code which is part of the 
	/// DocumentThread object.


	class ActionBase {
		public:
			/// Perform the action. This should update both the document
			/// tree data structure and the GUI. The latter is updated
			/// by calling relevant methods on the GUIBase object passed
			/// in.

			virtual void execute(DocumentThread&, GUIBase&)=0;

			/// Revert the change to the DTree document and the GUI.

			virtual void revert(DocumentThread&, GUIBase&)=0;

			/// Can this action be undone?
			virtual bool undoable() const;
	};
	
	/// \ingroup clientserver
	///
	/// Add a cell to the notebook.
	
	class ActionAddCell : public ActionBase {
		public:
			enum class Position { before, after, child };
			
			ActionAddCell(DataCell, DTree::iterator ref_, Position pos_);
			
			virtual void execute(DocumentThread&, GUIBase&) override;
			virtual void revert(DocumentThread&,  GUIBase&) override;
			
		private:
			// Keep track of the location where this cell is inserted into
			// the notebook. 
			
			DataCell          newcell;
			DTree::iterator   ref, newref;
			Position          pos;
			int               child_num;
	};


	/// \ingroup clientserver
	///
	/// Position the cursor relative to the indicated cell. If position is 'next' and
   /// there is no input cell following the indicated one, create a new one.

	class ActionPositionCursor : public ActionBase {
		public:
			enum class Position { in, next, previous };

			ActionPositionCursor(DTree::iterator ref_, Position pos_);

			virtual void execute(DocumentThread&, GUIBase&) override;
			virtual void revert(DocumentThread&,  GUIBase&) override;

		private:
			bool              needed_new_cell;
			DTree::iterator   ref, newref;
			Position          pos;
	};

	/// \ingroup clientserver
	///
	/// Update the running status of the indicated cell. 

	class ActionSetRunStatus : public ActionBase {
		public:
			ActionSetRunStatus(DTree::iterator ref_, bool running);

			virtual void execute(DocumentThread&, GUIBase&) override;
			virtual void revert(DocumentThread&,  GUIBase&) override;

			virtual bool undoable() const override;
		private:
			DTree::iterator this_cell;
			bool            was_running_, new_running_;
	};
	

	/// \ingroup clientserver
	///
	/// Remove a cell and all its child cells from the document.

	class ActionRemoveCell : public ActionBase {
		public:
			ActionRemoveCell(DTree::iterator ref_);
			~ActionRemoveCell();
			
			virtual void execute(DocumentThread&, GUIBase&) override;
			virtual void revert(DocumentThread&,  GUIBase&) override;

		private:
			// Keep track of the location where this cell (and its child
			// cells) was in the notebook.  We keep a reference to the
			// parent cell and the index of the current cell as child of
			// that parent.

			DTree             removed_tree;
			DTree::iterator   reference_parent_cell, this_cell;
			size_t            reference_child_index;
	};

	/// \ingroup clientserver
	///
	/// Split a cell into two separate cells, at the point of the cursor.

	class ActionSplitCell : public ActionBase {
		public:
			ActionSplitCell(DTree::iterator ref_);
			~ActionSplitCell();

			virtual void execute(DocumentThread&, GUIBase&) override;
			virtual void revert(DocumentThread&,  GUIBase&) override;
			
		private:
			DTree::iterator this_cell, newref; // the newly created cell
	};

	
	/// \ingroup clientserver
	///
	/// Add a text string (can be just a single character) at the point
	/// of the cursor.
	/// This action is assumed to be triggered from a user change to
	/// the GUI cells, so will not update the GUI itself, only the 
	/// underlying DTree. However, the revert method will need to
	/// update the GUI representation.

	class ActionInsertText : public ActionBase {
		public:
			ActionInsertText(DTree::iterator, int pos, const std::string&);
			
			virtual void execute(DocumentThread&, GUIBase&) override;
			virtual void revert(DocumentThread&,  GUIBase&) override;
			
		private:
			DTree::iterator   this_cell;
			int         insert_pos;
			std::string text;
	};
	
	/// \ingroup clientserver
	///
	/// Remove a text string starting at the indicated position, and
	/// with the indicated length, from the indicated cell.
	/// This action is assumed to be triggered from a user change to
	/// the GUI cells, so will not update the GUI itself, only the
	/// underlying DTree. However, the revert method will need to
	/// update the GUI representation.

	class ActionEraseText : public ActionBase {
		public:
			ActionEraseText(DTree::iterator, int, int);
			
			virtual void execute(DocumentThread&, GUIBase&) override;
			virtual void revert(DocumentThread&,  GUIBase&) override;
			
		private:
			DTree::iterator   this_cell;
			int from_pos, to_pos;
			std::string removed_text;
	};
	
}


//
//       class ActionMergeCells
			


