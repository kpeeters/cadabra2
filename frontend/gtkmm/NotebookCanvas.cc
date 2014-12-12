
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

bool NotebookCanvas::cell_content_changed()
	{
	std::cerr << "canvas received content changed" << std::endl;
	return false;
	}

bool NotebookCanvas::cell_content_execute(const std::string& content)
	{
	std::cerr << "canvas received content exec " << content << std::endl;
	return true;
	}

void NotebookCanvas::add_cell(DTree& tr, DTree::iterator it) 
	{
	// FIXME: handle other cell types.
	VisualCell newcell;
	Gtk::Widget *w=0;
	switch(it->cell_type) {
		case DataCell::CellType::output:
			newcell.outbox = manage( new TeXView(window.engine, it->textbuf) );
			w=newcell.outbox;
			break;
		case DataCell::CellType::input: {
			CodeInput *ci = new CodeInput();
			ci->edit.content_changed.connect( sigc::mem_fun(this, &NotebookCanvas::cell_content_changed) );
			ci->edit.content_execute.connect( sigc::mem_fun(this, &NotebookCanvas::cell_content_execute) );
			newcell.inbox = manage( ci );
			w=newcell.inbox;
			break;
			}
		default:
			throw std::logic_error("Unimplemented datacell type");
		}


	visualcells[&(*it)]=newcell;

	// Figure out where to store this new VisualCell in the GUI widget
	// tree by exploring the DTree near the new DataCell.

	DTree::iterator prev = DTree::previous_sibling(it);
	if(window.dtree().is_valid(prev.node)==false) {
		// This node has no previous sibling.
		DTree::iterator parent = DTree::parent(it);
		if(window.dtree().is_valid(parent)==false) {
			// This node has no parent either, it is a top-level cell.
			scrollbox.pack_start(*w, false, false);
			} 
		else {
			// Add as first child of parent.
			scrollbox.pack_start(*w, false, false);
			scrollbox.reorder_child(*w, 0);
			}
		}
	else {
		// Add in an existing sibling range.
		unsigned int index=tr.index(it);
		scrollbox.pack_start(*w, false, false);
		scrollbox.reorder_child(*w, index);
		}
	}
