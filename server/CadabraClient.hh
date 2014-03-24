
#pragma once

// This is a virtual base class for programs wanting to implement a 
// cadabra client. It contains the document representation, and manipulator
// functions to change it. It also contains the logic to execute a cell,
// that is, lock it, send it to the server, and wait for response.
// When a response comes in, the relevant virtual functions will be called.
// These should implement the necessary logic to update the display.
// On OS X these would then call Cocoa GCD to update the gui, whereas on
// gtkmm they would gdk_thread_enter and do the work.

class CadabraClient {
	public:
		
		void add_cell();
		void remove_cell();
		void split_cell();
		void execute_cell();
		
		virtual void on_progress()=0;
		virtual void on_tree_changed()=0;
		
	private:
		tree<Cell> doc;
};
