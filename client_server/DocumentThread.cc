
#include "DocumentThread.hh"
#include "Actions.hh"
#include "GUIBase.hh"

#include <iostream>

using namespace cadabra;

DocumentThread::DocumentThread(GUIBase* g)
	: gui(g), compute(0)
	{
	}

void DocumentThread::set_compute_thread(ComputeThread *cl)
	{
	compute = cl;
	}

void DocumentThread::new_document()
	{
	// Setup a single-cell document. This operation itself cannot be undone,
	// so we do it directly on the doc, not using Actions.

	DataCell top(DataCell::CellType::document);
	DTree::iterator doc_it = doc.set_head(top);
	gui->add_cell(doc, doc_it);

	// Three cells in the doc.

	DataCell one(DataCell::CellType::input, "");
	DTree::iterator one_it = doc.append_child(doc_it, one);
	gui->add_cell(doc, one_it);

	DataCell another(DataCell::CellType::input, "");
	DTree::iterator another_it = doc.insert_after(one_it, another);
	gui->add_cell(doc, another_it);

	DataCell out(DataCell::CellType::output, "$\\displaystyle\\int_{-\\infty}^\\infty A_{\\mu\\nu}$");
	DTree::iterator out_it = doc.insert_after(one_it, out);
	gui->add_cell(doc, out_it);
	}

const DTree& DocumentThread::dtree() 
	{
	return doc;
	}

void DocumentThread::queue_action(std::shared_ptr<ActionBase> ab) 
	{
	std::lock_guard<std::mutex> guard(stack_mutex);
	pending_actions.push(ab);
	}


void DocumentThread::process_action_queue()
	{
	std::cout << "Processing action queue" << std::endl;

	std::lock_guard<std::mutex> guard(stack_mutex);
	while(pending_actions.size()>0) {
		std::cout << "Action!" << std::endl;
		std::shared_ptr<ActionBase> ab = pending_actions.back();
		ab->execute(*this);
		ab->update_gui(doc, *gui);
		pending_actions.pop();
		}
	std::cout << "no more actions on pending queue" << std::endl;
	}
