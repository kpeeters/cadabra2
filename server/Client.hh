
#pragma once

// This is a virtual base class for programs wanting to implement a 
// cadabra client. It contains the document representation, and manipulator
// functions to change it. It also contains the logic to execute a cell,
// that is, lock it, send it to the server, and wait for response.
// When a response comes in, the relevant virtual functions will be called.
// These should implement the necessary logic to update the display.
// On OS X these would then call Cocoa GCD to update the gui, whereas on
// gtkmm they would gdk_thread_enter and do the work.

// Some of the functionality of this class is split off into Action classes 
// in order to provide an undo stack.

//FIX: do not use smart pointers, but instead address cells by iterators, and
//just use the fact that iterators remain valid even when nodes get moved in and 
//			  out of the tree. RemoveCell then just needs to keep track of the removed node.

namespace cadabra {

	class Client {
		public:
			Client();

			void run(); // runs websocket loop, how to start gtk loop? in a separate thread, see websocket++ docs
			void perform(ActionBase);

			virtual void on_progress()=0;
			virtual void on_tree_changed()=0;
			
			// DataCells are the basic building blocks for a document. They are stored 
			// in a tree inside the client. A user interface should read these cells
			// and construct corresponding graphical output for them.

//			class DataCell {
//				public:
//					enum class CellType { input, output, comment, texcomment, tex, error };
//					
//					DataCell(cell_t, const std::string& str="", bool texhidden=false);
//					
//					cell_t                        cell_type;
//					std::string                   textbuf;
//					std::string                   cdbbuf;             // c_output only: the output in cadabra input format
//					bool                          tex_hidden;         // c_tex only
//					bool                          sensitive;
//					bool                          running;
//			};
//			
//			// Do not manipulate this tree directly; instead submit ActionBase classes
//			// so that an undo stack can be kept.

			tree<DataCell> doc;

			// The Action object is used to pass user action instructions around
         // and store them in the undo/redo stacks. All references to cells is
         // in terms of smart pointers to DataCells. 

         // This requires that if we delete a cell, its data cell together
			// with any TextBuffer and TeXBuffer objects should be kept in
			// memory, so that the pointer remains valid.  We keep a RefPtr.

			class ActionBase {
				public:
					ActionBase(Glib::RefPtr<DataCell>);
					
					virtual void execute(XCadabra&)=0;
					virtual void revert(XCadabra&)=0;
					
					tree<DataCell>::iterator cell;
			};
			

//			class ActionAddCell : public ActionBase {
//				public:
//					enum class Position { before, after, child };
//
//					ActionAddCell(tree<DataCell>::iterator, tree<DataCell>::iterator ref_, Position pos_);
//					
//					/// Executing will also show the cell and grab its focus.
//					virtual void execute(XCadabra&);
//					virtual void revert(XCadabra&);
//					
//				private:
//					// Keep track of the location where this cell is inserted into
//					// the notebook. 
//
//					tree<DataCell>::iterator  ref;
//					Position                  position;
//			};
//			
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
//			// todo: split cell, execute cell (or should the latter be a normal, non-undoable function?)
			
		private:
	
			typedef std::stack<std::unique_ptr<ActionBase> > ActionStack;
			
	};

}
