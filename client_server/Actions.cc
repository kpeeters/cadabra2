

#include "Actions.hh"
#include "DataCell.hh"
#include "DocumentThread.hh"
#include "GUIBase.hh"

#include <iostream>

using namespace cadabra;

bool ActionBase::undoable() const 
	{
	return true;
	}

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
	// Remove the GUI cell from the notebook and then
	// remove the corresponding DataCell from the DTree.

	std::cerr << "removing cell " << newref->textbuf << std::endl;
	gb.remove_cell(cl.doc, newref);
	cl.doc.erase(newref);
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
				if(ref->textbuf=="") { // If the last cell is empty, stay where we are.
					newref=ref;
					}
				else {
					DataCell newcell(DataCell::CellType::python, "");
					newref = cl.doc.insert(sib, newcell);
					needed_new_cell=true;
					}
				}
			break;
			}
		case Position::previous: {
			bool found=false;
			DTree::sibling_iterator sib=ref;
			while(cl.doc.is_valid(--sib)) {
				if(sib->cell_type==DataCell::CellType::python || sib->cell_type==DataCell::CellType::latex) {
					if(!sib->hidden) {
						newref=sib;
						found=true;
						break;
						}
					}
				}
			if(!found) 
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
	if(needed_new_cell) {
		gb.remove_cell(cl.doc, newref);
		cl.doc.erase(newref);
		}
	gb.position_cursor(cl.doc, ref);
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

bool ActionSetRunStatus::undoable() const 
	{
	return false;
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


ActionInsertText::ActionInsertText(DTree::iterator ref_, int pos, const std::string& content)
	: this_cell(ref_), insert_pos(pos), text(content)
	{
	}

void ActionInsertText::execute(DocumentThread& cl, GUIBase& gb)  
	{
	this_cell->textbuf.insert(insert_pos, text);
	}

void ActionInsertText::revert(DocumentThread& cl, GUIBase& gb)
	{
	this_cell->textbuf.erase(insert_pos, text.size());
	gb.update_cell(cl.doc, this_cell);
	}


ActionEraseText::ActionEraseText(DTree::iterator ref_, int start, int end)
	: this_cell(ref_), from_pos(start), to_pos(end)
	{
	}

void ActionEraseText::execute(DocumentThread& cl, GUIBase& gb)  
	{
	std::cerr << from_pos << ", " << to_pos << std::endl;
	removed_text=this_cell->textbuf.substr(from_pos, to_pos-from_pos);
	this_cell->textbuf.erase(from_pos, to_pos-from_pos);
	}

void ActionEraseText::revert(DocumentThread& cl, GUIBase& gb)
	{
	this_cell->textbuf.insert(from_pos, removed_text);
	gb.update_cell(cl.doc, this_cell);
	}

