

#include "Actions.hh"
#include "DataCell.hh"
#include "DocumentThread.hh"
#include "GUIBase.hh"

#include <boost/core/demangle.hpp>
#include <iostream>

using namespace cadabra;

#define DEBUG(ln)
// #define DEBUG(ln) ln


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
	std::string class_name = boost::core::demangle(typeid(*this).name());
	throw std::logic_error(class_name + ": cannot find cell with id "+std::to_string(ref_id.id));
	}

ActionAddCell::ActionAddCell(DataCell cell, DataCell::id_t ref_id, Position pos_)
	: ActionBase(ref_id), newcell(cell), pos(pos_), is_replacement(false), is_input_form(false)
	{
	}

void ActionAddCell::execute(DocumentThread& cl, GUIBase& gb)
	{
	ActionBase::execute(cl, gb);

	// Insert this DataCell into the DTree document. We first need
	// to figure out whether we already have a cell with the DataCell's
	// cell_id; in this case we have to replace, not append/insert.
	auto it=cl.doc.begin();
	while(it!=cl.doc.end()) {
		if((*it).id().id==newcell.id().id) {
			// FIXME: right now we only change textbuf.
			DEBUG( std::cerr << "found! " << it->id().id << ", " << static_cast<int>(it->cell_type) << std::endl; )
			it->textbuf=newcell.textbuf;
			gb.update_cell(cl.doc, it);
			is_replacement=true;
			return;
			}
		++it;
		}

	// If we get here we have to append/insert.
	DEBUG( std::cerr << "ActionAddCell::execute: add cell with id " << newcell.id().id; )
	switch(pos) {
		case Position::before:
			newref = cl.doc.insert(ref, newcell);
			DEBUG( std::cerr << " before "; )
			break;
		case Position::after:
			newref = cl.doc.insert_after(ref, newcell);
			DEBUG( std::cerr << " after "; )			
			break;
		case Position::child:
			newref = cl.doc.append_child(ref, newcell);
			DEBUG( std::cerr << " as child of "; )			
			break;
		}
	DEBUG( std::cerr << "  " << ref->id().id << std::endl; )
		
	child_num=cl.doc.index(newref);
	DEBUG( std::cerr << "ActionAddCell::execute: added as child " << child_num <<
			 ": |" << newcell.textbuf << "|" << std::endl; )
	gb.add_cell(cl.doc, newref, true);

	if(newcell.cell_type == DataCell::CellType::input_form)
		is_input_form=true;
	}

void ActionAddCell::revert(DocumentThread& cl, GUIBase& gb)
	{
	// Remove the GUI cell from the notebook and then
	// remove the corresponding DataCell from the DTree.

	DEBUG( std::cerr << "ActionAddCell::revert: removing child " << child_num << std::endl; )

	DTree::sibling_iterator ch;
	switch(pos) {
		case Position::before:
			// `ref` is a cell after our cell.
			ch = cl.doc.child(cl.doc.parent(ref), child_num);
			break;
		case Position::after:
			// `ref` is a cell before our cell.
			ch = cl.doc.child(cl.doc.parent(ref), child_num);
			break;
		case Position::child:
			// `ref` is a parent of our cell.
			ch = cl.doc.child(ref, child_num);
			break;
		}
	DEBUG( std::cerr << "ActionAddCell::revert: removing cell " << ch->textbuf << std::endl; )
	gb.remove_cell(cl.doc, ch);
	// std::cerr << "ActionAddCell::revert: finally erase datacell" << std::endl;
	cl.doc.erase(ch);
	}

bool ActionAddCell::undoable() const
	{
	return !(is_replacement || is_input_form);
	}

ActionPositionCursor::ActionPositionCursor(DataCell::id_t ref_id, Position pos_)
	: ActionBase(ref_id), needed_new_cell_with_id(0), pos(pos_)
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
					}
				else {
					// Make sure that we store the generated cell id so we
					// can re-use it if we execute this in redo.
					if(needed_new_cell_with_id > 0) {
						DataCell::id_t id;
						id.id = needed_new_cell_with_id;
						DataCell newcell(id, DataCell::CellType::python, "");
						newref = cl.doc.insert(sib, newcell);
						}
					else {
						DataCell newcell(DataCell::CellType::python, "");
						needed_new_cell_with_id=newcell.id().id;
						newref = cl.doc.insert(sib, newcell);
						}
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
	if(needed_new_cell_with_id > 0) {
		// std::cerr << "cadabra-client: adding new visual cell before positioning cursor" << std::endl;
		gb.add_cell(cl.doc, newref, true);
		}
	// std::cerr << "cadabra-client: positioning cursor" << std::endl;
	gb.position_cursor(cl.doc, newref, -1);
	DEBUG( std::cerr << "ActionPositionCursor::execute: done" << std::endl; )
	}

void ActionPositionCursor::revert(DocumentThread& cl, GUIBase& gb)
	{
	if(needed_new_cell_with_id > 0) {
		gb.remove_cell(cl.doc, newref);
		cl.doc.erase(newref);
		}
	gb.position_cursor(cl.doc, ref, -1);
	DEBUG( std::cerr << "ActionPositionCursor::revert: done" << std::endl; )
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
	DEBUG( std::cerr << "removed has " << cl.doc.number_of_children(ref) << " children" << std::endl; )
	cl.doc.erase(ref);
	}

void ActionRemoveCell::revert(DocumentThread& cl, GUIBase& gb)
	{
	DEBUG( std::cerr << "need to undo a remove cell at index " << reference_child_index << std::endl; )
	DTree::iterator newcell;
	if(cl.doc.number_of_children(reference_parent_cell)==0) {
		newcell = cl.doc.append_child(reference_parent_cell, removed_tree.begin());
		}
	else {
		auto it = cl.doc.child(reference_parent_cell, reference_child_index);
		//		++it;
		newcell = cl.doc.insert_subtree(it, removed_tree.begin());
		DEBUG( std::cerr << "added doc cell " << newcell->textbuf << " at " << &(*newcell) << " before " << it->textbuf << std::endl; )
		}
	gb.add_cell(cl.doc, newcell, true);
	DEBUG( std::cerr << "added vis rep" << std::endl; )
	}


ActionReplaceCell::ActionReplaceCell(DataCell::id_t ref_id)
	: ActionBase(ref_id)
	{
	}

ActionReplaceCell::~ActionReplaceCell()
	{
	}

void ActionReplaceCell::execute(DocumentThread& cl, GUIBase& gb)
	{
	}

void ActionReplaceCell::revert(DocumentThread& cl, GUIBase& gb)
	{
	}

bool ActionReplaceCell::undoable() const
	{
	return false;
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

ActionSetVariableList::ActionSetVariableList(DataCell::id_t ref_id, std::set<std::string> variables)
	: ActionBase(ref_id), new_variables_(variables)
	{
	}

bool ActionSetVariableList::undoable() const
	{
	return false;
	}

void ActionSetVariableList::execute(DocumentThread& cl, GUIBase& gb)
	{
	ActionBase::execute(cl, gb);

	ref->variables_referenced=new_variables_;
	}

void ActionSetVariableList::revert(DocumentThread&, GUIBase& )
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
	DEBUG( std::cerr << "ActionInsertText::execute: textbuf now |" << ref->textbuf << "|" << std::endl; )
	gb.update_cell(cl.doc, ref);
	}

void ActionInsertText::revert(DocumentThread& cl, GUIBase& gb)
	{
	ref->textbuf.erase(insert_pos, text.size());
	DEBUG( std::cerr << "ActionInsertText::revert: textbuf now |" << ref->textbuf << "|" << std::endl; )
	gb.update_cell(cl.doc, ref);
	}


ActionCompleteText::ActionCompleteText(DataCell::id_t ref_id, int pos, const std::string& content, int alt)
	: ActionBase(ref_id), insert_pos(pos), text(content), alternative_(alt)
	{
	}

void ActionCompleteText::execute(DocumentThread& cl, GUIBase& gb)
	{
	ActionBase::execute(cl, gb);

	auto endpos = ref->textbuf.insert(insert_pos, text);
	// std::cerr << "complete: textbuf now |" << ref->textbuf << "|" << std::endl;
	gb.update_cell(cl.doc, ref);
	gb.position_cursor(cl.doc, ref, insert_pos+text.size());
	}

void ActionCompleteText::revert(DocumentThread& cl, GUIBase& gb)
	{
	ref->textbuf.erase(insert_pos, text.size());
	gb.update_cell(cl.doc, ref);
	gb.position_cursor(cl.doc, ref, insert_pos);
	}

int ActionCompleteText::length() const
	{
	return text.size();
	}

int ActionCompleteText::alternative() const
	{
	return alternative_;
	}

ActionEraseText::ActionEraseText(DataCell::id_t ref_id, int start, int end)
	: ActionBase(ref_id), from_pos(start), to_pos(end)
	{
	}

void ActionEraseText::execute(DocumentThread& cl, GUIBase& gb)
	{
	ActionBase::execute(cl, gb);

	DEBUG( std::cerr << from_pos << ", " << to_pos << std::endl; )
	removed_text=ref->textbuf.substr(from_pos, to_pos-from_pos);
	ref->textbuf.erase(from_pos, to_pos-from_pos);
	}

void ActionEraseText::revert(DocumentThread& cl, GUIBase& gb)
	{
	ref->textbuf.insert(from_pos, removed_text);
	gb.update_cell(cl.doc, ref);
	gb.position_cursor(cl.doc, ref, from_pos+removed_text.size());
	}

