
#include "NotebookCanvas.hh"
#include "NotebookWindow.hh"
#include <gtkmm/box.h>
#include <iostream>

using namespace cadabra;

NotebookCanvas::NotebookCanvas()
	{
//	pack1(scroll, true, true);
	pack_start(scroll);
 	scroll.set_policy(Gtk::POLICY_ALWAYS, Gtk::POLICY_ALWAYS);
 	scroll.set_border_width(1);
//	scroll.add(ebox);
//	ebox.override_background_color(Gdk::RGBA("white"));
	scroll.override_background_color(Gdk::RGBA("white"));
	}

NotebookCanvas::~NotebookCanvas()
	{
	}

