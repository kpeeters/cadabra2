
#pragma once

#include <deque>

#include "DataCell.hh"

namespace cadabra {

	/// \ingroup clientserver
	///
	/// Abstract base class with methods that need to be implemented
	/// by any GUI. You need to derive from this class as well as from
	/// the DocumentThread class.

	class GUIBase {
		public:
			/// The basic manipulations that a GUI needs to implement are
			/// adding, removing and updating (refreshing the display of)
			/// a cell. The code in DocumentThread will call these to make
			/// the GUI update its display. Called on the document thread.

			virtual void update_cell(const DTree&, DTree::iterator)=0;

			/// Remove a single cell together with all its child cells.
			/// Some toolkits (e.g. Gtk) will take care of that entire
			/// child tree removal automatically, in which case the only
			/// thing that needs done for the child cells is to remove
			/// any reference to their VisualCells.

			virtual void remove_cell(const DTree&, DTree::iterator)=0;

			/// Remove all GUI cells from the display (used as a quick way
			/// to clear all before loading a new document).

			virtual void remove_all_cells()=0;

			/// Add a GUI cell corresponding to the document cell at the
			/// iterator. The GUI needs to figure out from the location of
			/// this cell in the DTree where to insert the cell in the visual
			/// display. If the 'visible' flag is false, hide the cell from
			/// view independent of whether its hidden flag is set (this
			/// is only used when constructing a document on load time and
			/// we do not want to show cells until they have all been added
			/// to the document).

			virtual void add_cell(const DTree&, DTree::iterator, bool visible)=0;

			/// Position the cursor in the current canvas in the widget
			/// corresponding to the indicated cell.

			virtual void position_cursor(const DTree&, DTree::iterator, int)=0;

			/// Retrieve the position of the cursor in the current cell.

			virtual size_t get_cursor_position(const DTree&, DTree::iterator)=0;

			/// Network status is propagated from the ComputeThread to the
			/// GUI using the following methods. These get called on the
			/// compute thread (as opposed to the functions above, which get
			/// called on the gui thread).

			//@{

			virtual void on_connect()=0;
			virtual void on_disconnect(const std::string& reason)=0;
			virtual void on_network_error()=0;
			virtual void on_kernel_runstatus(bool)=0;

			//@}

			/// When the ComputeThread needs to modify the document, it
			/// stores an ActionBase object on the stack (see the
			/// DocumenThread class) and then wakes up the GUI thread
			/// signalling it to process this action. The following member
			/// should wake up the GUI thread and make it enter the
			/// processing part.

			virtual void process_data()=0;

		};

	};
