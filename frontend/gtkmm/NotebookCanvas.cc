
#include "NotebookCanvas.hh"
#include "NotebookWindow.hh"
#include <gtkmm/box.h>
#include <iostream>

using namespace cadabra;

NotebookCanvas::NotebookCanvas(NotebookWindow& w)
	: window(w)
	{
	pack1(scroll, true, true);
 	scroll.set_policy(Gtk::POLICY_ALWAYS, Gtk::POLICY_ALWAYS);
 	scroll.set_border_width(1);
	scroll.add(ebox);
	ebox.add(scrollbox);
	ebox.override_background_color(Gdk::RGBA("white"));
	}

NotebookCanvas::~NotebookCanvas()
	{
	}

void NotebookCanvas::add_cell(DTree::iterator it) 
	{
	// FIXME: handle other cell types.
	VisualCell newcell;
	newcell.outbox = manage( new TeXView(window.engine, it->textbuf) );
	visualcells[&(*it)]=newcell;

	// Figure out where to store this new VisualCell in the GUI widget
	// tree by exploring the DTree near the new DataCell.

	DTree::iterator prev = DTree::previous_sibling(it);
	if(window.dtree().is_valid(prev.node)==false) {
		// no previous sibling
		DTree::iterator parent = DTree::parent(it);
		if(window.dtree().is_valid(parent)==false) {
			// no parent either
			scrollbox.add(*newcell.outbox);
			} 
		else {
			// add as first child of parent
			}
		}
	else {
		}

//	Gtk::VBox::BoxList bl=scrollbox.children();
//	bl.insert(where, newone);// HERE


	}
