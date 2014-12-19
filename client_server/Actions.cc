

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
	: ref(ref_), pos(pos_)
	{
	}

void ActionPositionCursor::execute(DocumentThread& cl)  
	{
	std::cout << "ActionPositionCursor::execute" << std::endl;

	std::lock_guard<std::mutex> guard(cl.dtree_mutex);

	switch(pos) {
		case Position::in:
			newref = ref;
			break;
		case Position::next: {
			DTree::sibling_iterator sib=ref;
			++ref;
			if(cl.doc.is_valid(ref)==false) {
				DataCell newcell(DataCell::CellType::input, "");
				newref = cl.doc.insert_after(ref, newcell);
				}
			break;
			}
		case Position::previous:
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
	std::cout << "positioning cursor" << std::endl;
	gb.position_cursor(tr, newref);
	}
