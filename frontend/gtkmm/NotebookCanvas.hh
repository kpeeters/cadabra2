
#pragma once

#include <map>
#include <gtkmm/paned.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/separator.h>
#include <gtkmm/eventbox.h>

#include "VisualCell.hh"

/// NotebookCanvas is an actual view on the document. There can be any
/// number of them active inside the NotebookWindow. Each DataCell in the
/// notebook document has a corresponding VisualCell in the NotebookCanvas,
/// which gets added by NotebookCanvas::add_cell.
///
/// Cells which contain child cells (e.g. CodeInput, which can contain
/// child cells corresponding to the TeXView output) will also be
/// hierarchically ordered in the visual tree. That is, any visual cell
/// which can contain a child cell will have it stored inside a Gtk::Box
/// inside the visual cell. Removing any cell will therefore also
/// immediately remove the child cells.

namespace cadabra {

	class NotebookWindow;

	class NotebookCanvas : public Gtk::Paned {
		public:
			NotebookCanvas();
			~NotebookCanvas();

			std::map<DataCell *, VisualCell> visualcells;

			//			Gtk::EventBox             ebox;
			//			Gtk::VBox                 ebox;
			Gtk::EventBox             ebox;
			Gtk::ScrolledWindow       scroll;
			Gtk::Separator            bottomline;

			void refresh_all();

		};

	}
