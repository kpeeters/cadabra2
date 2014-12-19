

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

void ActionAddCell::execute(DocumentThread& cl)  
	{
	// std::cout << "ActionAddCell::execute" << std::endl;

	std::lock_guard<std::mutex> guard(cl.dtree_mutex);

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
	}

void ActionAddCell::revert(DocumentThread& cl)
	{
	// FIXME: implement
	}

void ActionAddCell::update_gui(const DTree& tr, GUIBase& gb)
	{
	std::cout << "updating gui for ActionAddCell" << std::endl;
	gb.add_cell(tr, newref);
	}


ActionPositionCursor::ActionPositionCursor(DTree::iterator ref_, Position pos_)
	: needed_new_cell(false), ref(ref_), pos(pos_)
	{
	}

void ActionPositionCursor::execute(DocumentThread& cl)  
	{
	// std::cout << "ActionPositionCursor::execute" << std::endl;

	std::lock_guard<std::mutex> guard(cl.dtree_mutex);

	switch(pos) {
		case Position::in:
			// std::cerr << "in" << std::endl;
			newref = ref;
			break;
		case Position::next: {
			DTree::sibling_iterator sib=ref;
			while(cl.doc.is_valid(++sib)) {
				if(sib->cell_type==DataCell::CellType::input) {
					// std::cout << "found input cell " << sib->textbuf << std::endl;
					newref=sib;
					return;
					}
				}
			// std::cout << "no input cell available, adding new" << std::endl;
			DataCell newcell(DataCell::CellType::input, "");
			newref = cl.doc.insert(sib, newcell);
			needed_new_cell=true;
			break;
			}
		case Position::previous:
			std::cerr << "previous" << std::endl;
			// FIXME: implement
			break;
		}
	}

void ActionPositionCursor::revert(DocumentThread& cl)  
	{
	// FIXME: implement
	}

void ActionPositionCursor::update_gui(const DTree& tr, GUIBase& gb)
	{
	if(needed_new_cell) {
		// std::cout << "adding new visual cell" << std::endl;
		gb.add_cell(tr, newref);
		}
	// std::cout << "positioning cursor" << std::endl;
	gb.position_cursor(tr, newref);
	}
