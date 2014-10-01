
#include "DocumentThread.hh"
#include "Actions.hh"
#include <iostream>

using namespace cadabra;


const DocumentThread::DTree& DocumentThread::dtree() 
	{
	return doc;
	}

void DocumentThread::execute_undo_stack_top()
	{
	if(undo_stack.size()>0) {
		std::cout << "executing an action" << std::endl;
		std::shared_ptr<ActionBase> act = undo_stack.top();
		act->execute(*this);
		// FIXME: we do not know about gui here
//		act->update_gui(*gui);
		}
	}

bool DocumentThread::perform(std::shared_ptr<ActionBase> ab) 
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

	// Put the ActionBase on the undo stack and execute it.
	undo_stack.push(ab);
	execute_undo_stack_top();
	
	return true;
	}
