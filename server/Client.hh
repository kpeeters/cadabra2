
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

namespace cadabra {

	class Client {
		public:
			
			void add_cell();
			void remove_cell();
			void split_cell();
			void execute_cell();
			
			virtual void on_progress()=0;
			virtual void on_tree_changed()=0;
			
			// DataCells are the basic building blocks for a document. They are stored 
			// in a tree inside the client. A user interface should read these cells
			// and construct corresponding graphical output for them.

			class DataCell {
				public:
					enum cell_t { c_input, c_output, c_comment, c_texcomment, c_tex, c_error };
					
					DataCell(cell_t, const std::string& str="", bool texhidden=false);
					
					cell_t                        cell_type;
					Glib::RefPtr<Gtk::TextBuffer> textbuf;
					Glib::RefPtr<TeXBuffer>       texbuf;
					std::string                   cdbbuf;             // c_output only: the output in cadabra input format
					bool                          tex_hidden;         // c_tex only
					bool                          sensitive;
					bool                          running;
			};
			
		private:
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
					
					Glib::RefPtr<DataCell> cell;
			};
			

			class ActionAddCell : public ActionBase {
				public:
					ActionAddCell(Glib::RefPtr<DataCell>, Glib::RefPtr<DataCell> ref_, bool before_);
					
					/// Executing will also show the cell and grab its focus.
					virtual void execute(XCadabra&);
					virtual void revert(XCadabra&);
					
				private:
					// Keep track of the location where this cell is inserted into
					// the notebook. 
					Glib::RefPtr<DataCell> ref;
					bool                   before;
					std::vector<Glib::RefPtr<DataCell> > associated_cells; // output and comment cells
			};
			
			class ActionRemoveCell : public ActionBase {
				public:
					ActionRemoveCell(Glib::RefPtr<DataCell>);
					~ActionRemoveCell();
					
					virtual void execute(XCadabra&);
					virtual void revert(XCadabra&);
					
				private:
					// Keep track of the location where this cell was in the notebook. Since it is
					// not possible to delete the first cell, it is safe to keep a reference to the
					// cell just before the one we are deleting. 
					// Note that since we keep a RefPtr to this datacell, that cell will stay alive
					// even when a subsequent action will remove it. 
					Glib::RefPtr<DataCell>               prev_cell; 
					std::vector<Glib::RefPtr<DataCell> > associated_cells; // output and comment cells
			};
			
			class ActionAddText : public ActionBase {
				public:
					ActionAddText(Glib::RefPtr<DataCell>, int, const std::string&);
					
					virtual void execute(XCadabra&);
					virtual void revert(XCadabra&);
					
					int         insert_pos;
					std::string text;
			};
			
			class ActionRemoveText : public ActionBase {
				public:
					ActionRemoveText(Glib::RefPtr<DataCell>, int, int, const std::string&);
					
					virtual void execute(XCadabra&);
					virtual void revert(XCadabra&);
					
					int from_pos, to_pos;
					std::string removed_text;
			};
			
			typedef std::stack<Glib::RefPtr<ActionBase> > ActionStack;
			
	};

}
