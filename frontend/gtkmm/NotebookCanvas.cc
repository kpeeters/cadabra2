
#include "NotebookCanvas.hh"
#include "NotebookWindow.hh"
#include <gtkmm/box.h>
#include <iostream>

using namespace cadabra;

NotebookCanvas::NotebookCanvas()
	{
	// Pack the scroll widget with all document cells into the top pane.
	pack1(ebox, true, true);
	ebox.add(scroll);
 	scroll.set_policy(Gtk::POLICY_ALWAYS, Gtk::POLICY_ALWAYS);
 	scroll.set_border_width(1);
//	scroll.add(ebox);
//	ebox.override_background_color(Gdk::RGBA("white"));

	// Do NOT do the following. This will create areas at the top
	// and bottom where the content of the scrolledwindow is
	// covered with white (except when totally at the top or
	// bottom of the content).
        // scroll.override_background_color(Gdk::RGBA("white"));
	(*this).override_background_color(Gdk::RGBA("white"));

	ebox.set_events(Gdk::SCROLL_MASK | Gdk::SMOOTH_SCROLL_MASK | Gdk::BUTTON_PRESS_MASK);
//	scroll.set_overlay_scrolling(false);
	}

NotebookCanvas::~NotebookCanvas()
	{
	}

void NotebookCanvas::refresh_all()
	{
	auto it=visualcells.begin();
	while(it!=visualcells.end()) {
		auto ct=it->first->cell_type;
		if(ct==DataCell::CellType::output || ct==DataCell::CellType::latex_view) {
			it->second.outbox->update_image();
			}
		++it;
		}
	}
