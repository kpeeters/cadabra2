
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

#include "tree.hh"
#include <stack>

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/functional.hpp>

typedef websocketpp::client<websocketpp::config::asio_client> WSClient;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;
typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;

namespace cadabra {

	class GUIBase;

	class Client {
		public:
			// If the Client is constructed with a null pointer to the gui,
			// there will be no gui updates, just DTree updates.

			Client(GUIBase *);
			Client(const Client& )=delete; // You cannot copy this object
			~Client();

			// Main entry point, which will connect to the server and then start an
			// event loop to handle communication with the server. Only terminates when
			// the connection drops. Run your GUI on a different thread.

			void run(); 

			// All modifications to the document are done by calling 'perform' with an 
			// action object. This enables us to implement an undo stack. This method
			// will take care of making the actual change to the DTree document, and
			// call back on the 'change' methods above to inform the derived class
			// that a change has been made. 
			//
			// Perform calls can be made both by the GUI (in response to user input)
			// and by this Client itself (in response to server results coming back).
			//
			// The return value is true unless the action was refused (e.g. trying to
			// delete a cell which is locked against deletion).
			//
			// The 'perform' method is synchronous and 'fast', in the sense
			// that it can run on the UI thread. In contrast to
			// 'run_cell', which is asynchronous and fast.

			class ActionBase;
			bool perform(std::shared_ptr<ActionBase>);

			// DataCells are the basic building blocks for a document. They are stored 
			// in a tree inside the client. A user interface should read these cells
			// and construct corresponding graphical output for them.

			class DataCell {
				public:
					enum class CellType { input, output, comment, texcomment, tex, error };
					
					DataCell(CellType t=CellType::input, const std::string& str="", bool texhidden=false);
					
					CellType                      cell_type;
					std::string                   textbuf;
					std::string                   cdbbuf;             // c_output only: the output in cadabra input format
					bool                          tex_hidden;         // c_tex only
					bool                          sensitive;
					bool                          running;
			};
			
			// The document is a tree of DataCells. A read-only version of this document
			// is available from the 'dtree' function. All changes to the tree should be
			// made by submitting ActionBase derived objects to the 'perform' function,
			// so that an undo stack can be kept.

			typedef tree<std::shared_ptr<DataCell> >  DTree;
			typedef DTree::iterator                   iterator;

			std::mutex   dtree_mutex;
			const DTree& dtree();

			// The Action object is used to pass user action instructions around
         // and store them in the undo/redo stacks. All references to cells is
         // in terms of iterators to DataCells. 

         // This requires that if we delete a cell, its data cell together
			// with any TextBuffer and TeXBuffer objects should be kept in
			// memory, so that the pointer remains valid.  We keep a RefPtr.

			class ActionBase {
				public:
					ActionBase();

					virtual void execute(Client&)=0;

					virtual void revert(Client&)=0;
					virtual void update_gui(GUIBase&)=0;
			};

			class ActionAddCell : public ActionBase {
				public:
					enum class Position { before, after, child };
					
					ActionAddCell(std::shared_ptr<DataCell>, iterator ref_, Position pos_);
					
					virtual void execute(Client&);
					virtual void revert(Client&);

					virtual void update_gui(GUIBase&);

				private:
					// Keep track of the location where this cell is inserted into
					// the notebook. 

					std::shared_ptr<DataCell>  datacell;
					iterator                   ref, newcell;
					Position                   pos;
			};
			
			// The command pattern implemented by the objects derived from ActionBase is
			// acting on the DTree. Actions on the GUI elements (not present in Client but
			// in a separate object deriving from GUIBase) is implemented by these objects
			// calling methods in GUIBase, with the required parameters.


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
			

			
			// Finally, the logic to run_cell code in cells. This is a normal function as it
			// cannot be undone anyway so it is pointless to put it in the undo stack.
			// If you want to undo an action, you need to restart the kernel on the server.
			//
			// Once run_cell is called on a cell, the cell is locked against deletion.
			//
			// This method returns as soon as the request has been put on the network queue.
			// The result of the computation will be reported through one of the callback
			// members (on_network_error, before_tree_change, ...) defined earlier.

			void run_cell(iterator);

		private:
			GUIBase *gui;

			// WebSocket++ things.
			WSClient wsclient;
			websocketpp::connection_hdl our_connection_hdl;
			void init();
			void on_open(websocketpp::connection_hdl hdl);
			void on_fail(websocketpp::connection_hdl hdl);
			void on_close(websocketpp::connection_hdl hdl);
			void on_message(websocketpp::connection_hdl hdl, message_ptr msg);

			// The actual document, the actions that led to it, and mutexes for
			// locking.
			DTree doc;

			typedef std::stack<std::shared_ptr<ActionBase> > ActionStack;
			ActionStack undo_stack, redo_stack;

			// Execution logic.
			void execute_undo_stack_top();
	};

}
