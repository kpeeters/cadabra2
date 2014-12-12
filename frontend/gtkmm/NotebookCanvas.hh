
#pragma once

#include <map>
#include <gtkmm/paned.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/separator.h>
#include <gtkmm/eventbox.h>

// NotebookCanvas is an actual view on the document. There can be any
// number of them active inside the NotebookWindow.

#include "VisualCell.hh"

namespace cadabra {

	class NotebookWindow;
	
	class NotebookCanvas : public Gtk::VPaned {
		public:
			NotebookCanvas(NotebookWindow& doc);
			~NotebookCanvas();
			
         //	bool handle_key_press_event(GdkEventKey*);

			// Three members similar to those in GUIBase. They get called
			// not from DocumentThread but from NotebookCanvas.

			virtual void add_cell(DTree&, DTree::iterator);

		private:
			NotebookWindow&  window;

			std::map<DataCell *, VisualCell> visualcells;

			Gtk::EventBox             ebox;
			Gtk::ScrolledWindow       scroll;
			Gtk::VBox                 scrollbox;
			Gtk::HSeparator           bottomline;

			// The following are handlers that get called when the content
			// of a cell is changed or the user requests to run it (shift-enter).

			bool cell_content_changed();
			bool cell_content_execute(const std::string&);
	};

}
