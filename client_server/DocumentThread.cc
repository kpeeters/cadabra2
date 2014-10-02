
#include "DocumentThread.hh"
#include "Actions.hh"
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
	// Setup a single-cell document.
	

//	std::lock_guard<std::mutex> guard(client->dtree_mutex);
//
//	Client::iterator it=client->dtree().begin();
//	auto newcell = std::make_shared<Client::DataCell>();
//	auto ac = std::make_shared<Client::ActionAddCell>(newcell, 
//																				  it, 
//																				  ActionAddCell::Position::child);
//
//	client->perform(ac);
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
//	// FIXME: this is just a test action
//	std::string msg = 
//		"{ \"header\":   { \"uuid\": \"none\", \"msg_type\": \"execute_request\" },"
//		"  \"content\":  { \"code\": \"import time\nprint(42)\ntime.sleep(10)\n\"} "
//		"}";
////	std::cout << "sending" << std::endl;
//	wsclient.send(our_connection_hdl, msg, websocketpp::frame::opcode::text);
////	std::cout << "sending done" << std::endl;
//
//	// Now update the gui:
////	ab.update_gui(gui);
//
	
	std::lock_guard<std::mutex> guard(stack_mutex);
	while(pending_actions.size()>0) {
		std::shared_ptr<ActionBase> ab = pending_actions.back();
		ab->execute(*this);
		ab->update_gui(*gui);
		pending_actions.pop();
		}
	std::cout << "no more actions on pending queue" << std::endl;
	}
