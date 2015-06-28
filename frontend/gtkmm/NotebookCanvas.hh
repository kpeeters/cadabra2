
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
			NotebookCanvas();
			~NotebookCanvas();
			
			std::map<DataCell *, VisualCell> visualcells;

//			Gtk::EventBox             ebox;
//			Gtk::VBox                 ebox;
			Gtk::ScrolledWindow       scroll;
			Gtk::HSeparator           bottomline;

			void refresh_all();

	};

}
