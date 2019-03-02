

#include "Actions.hh"
#include "DataCell.hh"
#include "DocumentThread.hh"
#include "GUIBase.hh"

#include <iostream>

using namespace cadabra;

ActionBase::ActionBase(DataCell::id_t id)
	: ref_id(id)
	{
	}

bool ActionBase::undoable() const
	{
	return true;
	}

void ActionBase::execute(DocumentThread& cl, GUIBase& )
	{
	auto it=cl.doc.begin();
	while(it!=cl.doc.end()) {
		if((*it).id().id==ref_id.id) {
			ref=it;
			return;
			}
		++it;
		}
	throw std::logic_error("ActionAddCell: cannot find cell with id "+std::to_string(ref_id.id));
	}

ActionAddCell::ActionAddCell(DataCell cell, DataCell::id_t ref_id, Position pos_)
	: ActionBase(ref_id), newcell(cell), pos(pos_)
	{
	}

void ActionAddCell::execute(DocumentThread& cl, GUIBase& gb)
	{
	ActionBase::execute(cl, gb);

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
	child_num=cl.doc.index(newref);
	gb.add_cell(cl.doc, newref, true);
	}

void ActionAddCell::revert(DocumentThread& cl, GUIBase& gb)
	{
	// Remove the GUI cell from the notebook and then
	// remove the corresponding DataCell from the DTree.

	auto ch = cl.doc.child(ref, child_num);
	//std::cerr << "removing cell " << ch->textbuf << std::endl;
	gb.remove_cell(cl.doc, ch);
	cl.doc.erase(ch);
	}


ActionPositionCursor::ActionPositionCursor(DataCell::id_t ref_id, Position pos_)
	: ActionBase(ref_id), needed_new_cell(false), pos(pos_)
	{
	}

void ActionPositionCursor::execute(DocumentThread& cl, GUIBase& gb)
	{
	ActionBase::execute(cl, gb);

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
				} else {
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
	gb.position_cursor(cl.doc, newref, -1);
	}

void ActionPositionCursor::revert(DocumentThread& cl, GUIBase& gb)
	{
	if(needed_new_cell) {
		gb.remove_cell(cl.doc, newref);
		cl.doc.erase(newref);
		}
	gb.position_cursor(cl.doc, ref, -1);
	}


ActionRemoveCell::ActionRemoveCell(DataCell::id_t ref_id)
	: ActionBase(ref_id)
	{
	}

ActionRemoveCell::~ActionRemoveCell()
	{
	}

void ActionRemoveCell::execute(DocumentThread& cl, GUIBase& gb)
	{
	ActionBase::execute(cl, gb);

	gb.remove_cell(cl.doc, ref);

	reference_parent_cell = cl.doc.parent(ref);
	reference_child_index = cl.doc.index(ref);
	removed_tree=DTree(ref);
	//	std::cerr << "removed has " << cl.doc.number_of_children(ref) << " children" << std::endl;
	cl.doc.erase(ref);
	}

void ActionRemoveCell::revert(DocumentThread& cl, GUIBase& gb)
	{
	//std::cerr << "need to undo a remove cell at index " << reference_child_index << std::endl;
	DTree::iterator newcell;
	if(cl.doc.number_of_children(reference_parent_cell)==0) {
		newcell = cl.doc.append_child(reference_parent_cell, removed_tree.begin());
		} else {
		auto it = cl.doc.child(reference_parent_cell, reference_child_index);
		//		++it;
		newcell = cl.doc.insert_subtree(it, removed_tree.begin());
		//		std::cerr << "added doc cell " << newcell->textbuf << " at " << &(*newcell) << " before " << it->textbuf << std::endl;
		}
	gb.add_cell(cl.doc, newcell, true);
	//std::cerr << "added vis rep" << std::endl;
	}


ActionSplitCell::ActionSplitCell(DataCell::id_t ref_id)
	: ActionBase(ref_id)
	{
	}

ActionSplitCell::~ActionSplitCell()
	{
	}

void ActionSplitCell::execute(DocumentThread& cl, GUIBase& gb)
	{
	ActionBase::execute(cl, gb);

	size_t pos = gb.get_cursor_position(cl.doc, ref);

	std::string segment1=ref->textbuf.substr(0, pos);
	std::string segment2=ref->textbuf.substr(pos);

	// Strip leading newline in 2nd segment, if any.
	if(segment2.size()>0) {
		if(segment2[0]=='\n')
			segment2=segment2.substr(1);
		}

	DataCell newcell(ref->cell_type, segment2);
	newref = cl.doc.insert_after(ref, newcell);
	ref->textbuf=segment1;

	gb.add_cell(cl.doc, newref, true);
	gb.update_cell(cl.doc, ref);
	}

void ActionSplitCell::revert(DocumentThread&, GUIBase& )
	{
	// FIXME: implement
	}



ActionSetRunStatus::ActionSetRunStatus(DataCell::id_t ref_id, bool running)
	: ActionBase(ref_id), new_running_(running)
	{
	}

bool ActionSetRunStatus::undoable() const
	{
	return false;
	}

void ActionSetRunStatus::execute(DocumentThread& cl, GUIBase& gb)
	{
	ActionBase::execute(cl, gb);

	gb.update_cell(cl.doc, ref);

	was_running_=ref->running;
	ref->running=new_running_;
	}

void ActionSetRunStatus::revert(DocumentThread&, GUIBase& )
	{
	}


ActionInsertText::ActionInsertText(DataCell::id_t ref_id, int pos, const std::string& content)
	: ActionBase(ref_id), insert_pos(pos), text(content)
	{
	}

void ActionInsertText::execute(DocumentThread& cl, GUIBase& gb)
	{
	ActionBase::execute(cl, gb);

	ref->textbuf.insert(insert_pos, text);
	}

void ActionInsertText::revert(DocumentThread& cl, GUIBase& gb)
	{
	ref->textbuf.erase(insert_pos, text.size());
	gb.update_cell(cl.doc, ref);
	}


ActionEraseText::ActionEraseText(DataCell::id_t ref_id, int start, int end)
	: ActionBase(ref_id), from_pos(start), to_pos(end)
	{
	}

void ActionEraseText::execute(DocumentThread& cl, GUIBase& gb)
	{
	ActionBase::execute(cl, gb);

	//std::cerr << from_pos << ", " << to_pos << std::endl;
	removed_text=ref->textbuf.substr(from_pos, to_pos-from_pos);
	ref->textbuf.erase(from_pos, to_pos-from_pos);
	}

void ActionEraseText::revert(DocumentThread& cl, GUIBase& gb)
	{
	ref->textbuf.insert(from_pos, removed_text);
	gb.update_cell(cl.doc, ref);
	gb.position_cursor(cl.doc, ref, from_pos+removed_text.size());
	}

