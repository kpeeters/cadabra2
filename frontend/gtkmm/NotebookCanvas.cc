
#include "NotebookCanvas.hh"
#include "NotebookWindow.hh"
#include <gtkmm/box.h>
#include <iostream>

using namespace cadabra;

NotebookCanvas::NotebookCanvas()
	{
	// Pack the scroll widget with all document cells into the top pane.
	pack1(scroll, true, true);
 	scroll.set_policy(Gtk::POLICY_ALWAYS, Gtk::POLICY_ALWAYS);
 	scroll.set_border_width(1);
//	scroll.add(ebox);
//	ebox.override_background_color(Gdk::RGBA("white"));
	scroll.override_background_color(Gdk::RGBA("white"));
	}

NotebookCanvas::~NotebookCanvas()
	{
	}

void NotebookCanvas::refresh_all()
	{
	auto it=visualcells.begin();
	while(it!=visualcells.end()) {
		if(it->first->cell_type==DataCell::CellType::output) {
			it->second.outbox->update_image();
			}
		++it;
		}
	}
