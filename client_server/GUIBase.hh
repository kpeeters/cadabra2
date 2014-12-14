
#pragma once

#include <deque>

#include "DataCell.hh"

// Abstract base class with methods that need to be implemented
// by any GUI. You need to derive from this class as well as from
// the DocumentThread class.

namespace cadabra {
	
	class GUIBase {
		public:
			// The basic manipulations that a GUI needs to implement are
			// adding, removing and updating (refreshing the display of)
			// a cell. The code in DocumentThread will call these to make
			// the GUI update its display. Called on the document thread.

			virtual void update_cell(DTree&, DTree::iterator)=0;
			virtual void remove_cell(DTree&, DTree::iterator)=0;

			// Add a GUI cell corresponding to the document cell at the
			// iterator. The GUI needs to figure out from the location of
			// this cell in the DTree where to insert the cell in the visual
			// display.
			virtual void add_cell(const DTree&, DTree::iterator)=0;

			// Network status is propagated from the ComputeThread to the
         // GUI using the following methods. These get called on the
			// compute thread.

			virtual void on_connect()=0;
			virtual void on_disconnect()=0;
			virtual void on_network_error()=0;

			// When the ComputeThread needs to modify the document, it
			// stores an ActionBase object on the stack (see the
			// DocumenThread class) and then wakes up the GUI thread
			// signalling it to process this action. The following member
			// should wake up the GUI thread and make it enter the 
			// processing part.

			virtual void process_data()=0;

	};

};
