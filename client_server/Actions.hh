
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

	class ActionBase {
		public:
			virtual void execute(DocumentThread&)=0;
			virtual void revert(DocumentThread&)=0;
			virtual void update_gui(GUIBase&)=0;
	};
	
	// The Action object is used to pass user action instructions around
	// and store them in the undo/redo stacks. 
	
	// This requires that if we delete a cell, its data cell together
	// with any TextBuffer and TeXBuffer objects should be kept in
	// memory, so that the pointer remains valid. We keep a shared_ptr.
	
	// All actions run on the GUI thread.
	
	
	class ActionAddCell : public ActionBase {
		public:
			enum class Position { before, after, child };
			
			ActionAddCell(DTree::iterator, DTree::iterator ref_, Position pos_);
			
			virtual void execute(DocumentThread&);
			virtual void revert(DocumentThread&);
			virtual void update_gui(GUIBase&);
			
		private:
			// Keep track of the location where this cell is inserted into
			// the notebook. 
			
			DTree::iterator   ref, newcell;
			Position          pos;
	};
	
}	
	
//			class ActionRemoveCell : public ActionBase {
//				public:
//					ActionRemoveCell(tree<DataCell>::iterator);
//					~ActionRemoveCell();
//					
//					virtual void execute(XCadabra&);
//					virtual void revert(XCadabra&);
//					
//				private:
//					// Keep track of the location where this cell was in the notebook. Since it is
//					// not possible to delete the first cell, it is safe to keep a reference to the
//					// cell just before the one we are deleting. 
//
//					tree<DataCell>::iterator             prev_cell;
//			};
//			
//			class ActionAddText : public ActionBase {
//				public:
//					ActionAddText(tree<DataCell>::iterator, int, const std::string&);
//					
//					virtual void execute(XCadabra&);
//					virtual void revert(XCadabra&);
//					
//					int         insert_pos;
//					std::string text;
//			};
//			
//			class ActionRemoveText : public ActionBase {
//				public:
//					ActionRemoveText(tree<DataCell>::iterator, int, int, const std::string&);
//					
//					virtual void execute(XCadabra&);
//					virtual void revert(XCadabra&);
//					
//					int from_pos, to_pos;
//					std::string removed_text;
//			};
//
//			class ActionSplitCell
//       class ActionMergeCells
			

