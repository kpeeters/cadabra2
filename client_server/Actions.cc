

#include "Actions.hh"
#include "DataCell.hh"
#include "DocumentThread.hh"

using namespace cadabra;

ActionAddCell::ActionAddCell(std::shared_ptr<DataCell> dc, DocumentThread::iterator ref_, Position pos_) 
	: datacell(dc), ref(ref_), pos(pos_)
	{
	}

void ActionAddCell::execute(ActionBase& cl)  
	{
	std::cout << "locking tree" << std::endl;
	std::lock_guard<std::mutex> guard(cl.dtree_mutex);
	std::cout << "tree locked" << std::endl;

// HERE, need to be able to create an empty document etc...
	newcell = cl.doc.append_child(ref, datacell);
	}

void ActionAddCell::revert(ActionBase& cl)
	{
	}

void ActionAddCell::update_gui(GUIBase& gb)
	{
	std::cout << "updating gui for ActionAddCell" << std::endl;
	GUIBase::GUIAction action(GUIBase::GUIAction::Type::ADD, newcell);

	std::lock_guard<std::mutex> guard(gb.gui_todo_mutex);
	gb.gui_todo_deque.push_back(action);
	gb.new_todo_notification();
	}
