
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
//	ebox.modify_bg(Gtk::STATE_NORMAL, Gdk::Color("white"));
	}

NotebookCanvas::~NotebookCanvas()
	{
	}

void NotebookCanvas::add_cell(DTree::iterator it) 
	{
	std::cout << "adding cell to canvas" << std::endl;
	// FIXME: handle other cell types.

	VisualCell newcell;
	visualcells[&(*it)]=newcell;

	DTree::iterator prev = DTree::previous_sibling(it);
	if(window.dtree().is_valid(prev.node)==false) {
		std::cout << "no previous sibling" << std::endl;
		}
	
   // where do we store these VisualCells?
	// If we would just keep a map from datacell to visualcell, then we
	// could first move to the previous_sibling of this data cell, figure out
	// its visualcell, then add 

//	Gtk::VBox::BoxList bl=scrollbox.children();
//	bl.insert(where, newone);// HERE


	}
