

#include "Actions.hh"
#include "DataCell.hh"
#include "DocumentThread.hh"
#include "GUIBase.hh"

#include <iostream>

using namespace cadabra;

ActionAddCell::ActionAddCell(DataCell cell, DTree::iterator ref_, Position pos_) 
	: newcell(cell), ref(ref_), pos(pos_)
	{
	}

void ActionAddCell::execute(DocumentThread& cl, GUIBase& gb)  
	{
	// std::cout << "ActionAddCell::execute" << std::endl;

//	std::lock_guard<std::mutex> guard(cl.dtree_mutex);

	// Insert this DataCell into the DTree document.
	switch(pos) {
		case Position::before:
			newref = cl.doc.insert(ref, newcell);
			break;
		case Position::after:
			newref = cl.doc.insert_after(ref, newcell);
			break;
		case Position::child:
			newref = cl.doc.append_child(ref, newcell);
			break;
		}
	gb.add_cell(cl.doc, newref, true);
	}

void ActionAddCell::revert(DocumentThread& cl, GUIBase& gb)
	{
	// FIXME: implement
	}


ActionPositionCursor::ActionPositionCursor(DTree::iterator ref_, Position pos_)
	: needed_new_cell(false), ref(ref_), pos(pos_)
	{
	}

void ActionPositionCursor::execute(DocumentThread& cl, GUIBase& gb)  
	{
	// std::cout << "ActionPositionCursor::execute" << std::endl;

//	std::lock_guard<std::mutex> guard(cl.dtree_mutex);

	switch(pos) {
		case Position::in:
			// std::cerr << "in" << std::endl;
			newref = ref;
			break;
		case Position::next: {
			DTree::sibling_iterator sib=ref;
			bool found=false;
			while(cl.doc.is_valid(++sib)) {
				if(sib->cell_type==DataCell::CellType::python || sib->cell_type==DataCell::CellType::latex) {
					if(!sib->hidden) {
						newref=sib;
						found=true;
						break;
						}
					}
				}
			if(!found) {
				if(ref->textbuf!="") { // If the last cell is empty, stay where we are.
					DataCell newcell(DataCell::CellType::python, "");
					newref = cl.doc.insert(sib, newcell);
					needed_new_cell=true;
					}
				}
			break;
			}
		case Position::previous: {
			DTree::sibling_iterator sib=ref;
			while(cl.doc.is_valid(--sib)) {
				if(sib->cell_type==DataCell::CellType::python || sib->cell_type==DataCell::CellType::latex) {
					if(!sib->hidden) {
						newref=sib;
						return;
						}
					}
				}
			newref=ref; // No previous sibling cell. FIXME: walk tree structure
			break;
			}
		}

	// Update GUI.
	if(needed_new_cell) {
		// std::cerr << "cadabra-client: adding new visual cell before positioning cursor" << std::endl;
		gb.add_cell(cl.doc, newref, true);
		}
	// std::cerr << "cadabra-client: positioning cursor" << std::endl;
	gb.position_cursor(cl.doc, newref);
	}

void ActionPositionCursor::revert(DocumentThread& cl, GUIBase& gb)  
	{
	// FIXME: implement
	}


ActionRemoveCell::ActionRemoveCell(DTree::iterator ref_) 
	: this_cell(ref_)
	{
	}

ActionRemoveCell::~ActionRemoveCell()
	{
	}

void ActionRemoveCell::execute(DocumentThread& cl, GUIBase& gb)  
	{
	gb.remove_cell(cl.doc, this_cell);

	reference_parent_cell = cl.doc.parent(this_cell);
	reference_child_index = cl.doc.index(this_cell);
	cl.doc.erase(this_cell);
	}

void ActionRemoveCell::revert(DocumentThread& cl, GUIBase& gb)
	{
	// FIXME: implement
	}


ActionSplitCell::ActionSplitCell(DTree::iterator ref_) 
	: this_cell(ref_)
	{
	}

ActionSplitCell::~ActionSplitCell()
	{
	}

void ActionSplitCell::execute(DocumentThread& cl, GUIBase& gb)  
	{
//	std::lock_guard<std::mutex> guard(cl.dtree_mutex);

	size_t pos = gb.get_cursor_position(cl.doc, this_cell);

	std::string segment1=this_cell->textbuf.substr(0, pos);
	std::string segment2=this_cell->textbuf.substr(pos);

	// Strip leading newline in 2nd segment, if any.
	if(segment2.size()>0) {
		if(segment2[0]=='\n')
			segment2=segment2.substr(1);
		}

	DataCell newcell(this_cell->cell_type, segment2);
	newref = cl.doc.insert_after(this_cell, newcell);
	this_cell->textbuf=segment1;

	gb.add_cell(cl.doc, newref, true);
	gb.update_cell(cl.doc, this_cell);
	}

void ActionSplitCell::revert(DocumentThread& cl, GUIBase& gb)
	{
	// FIXME: implement
	}



ActionSetRunStatus::ActionSetRunStatus(DTree::iterator ref_, bool running) 
	: this_cell(ref_), new_running_(running)
	{
	}

void ActionSetRunStatus::execute(DocumentThread& cl, GUIBase& gb)  
	{
	gb.update_cell(cl.doc, this_cell);

	was_running_=this_cell->running;
	this_cell->running=new_running_;
	}

void ActionSetRunStatus::revert(DocumentThread& cl, GUIBase& gb)
	{
	}

