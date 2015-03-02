
#include <string>
#include <iostream>
#include <sstream>
#include "ComputeThread.hh"
#include "DocumentThread.hh"
#include "GUIBase.hh"
#include "Actions.hh"
#include "spawn.h"

using namespace cadabra;
typedef websocketpp::client<websocketpp::config::asio_client> client;

ComputeThread::ComputeThread(GUIBase *g, DocumentThread& dt)
	: gui(g), docthread(dt), connection_is_open(false)
	{
	}

ComputeThread::~ComputeThread()
	{
	}

void ComputeThread::init() 
	{
	// Setup the WebSockets client.

	wsclient.init_asio();
	wsclient.start_perpetual();
	}

void ComputeThread::try_connect()
	{
	using websocketpp::lib::placeholders::_1;
	using websocketpp::lib::placeholders::_2;
	using websocketpp::lib::bind;

    wsclient.clear_access_channels(websocketpp::log::alevel::all);
	wsclient.clear_error_channels(websocketpp::log::elevel::all);

	wsclient.set_open_handler(bind(&ComputeThread::on_open, this, ::_1));
	wsclient.set_fail_handler(bind(&ComputeThread::on_fail, this, ::_1));
	wsclient.set_close_handler(bind(&ComputeThread::on_close, this, ::_1));
	wsclient.set_message_handler(bind(&ComputeThread::on_message, this, ::_1, ::_2));

	std::string uri = "ws://localhost:9002";
	websocketpp::lib::error_code ec;
	connection = wsclient.get_connection(uri, ec);
	if (ec) {
		std::cerr << "error: " << ec.message() << std::endl;
		return;
		}

	our_connection_hdl = connection->get_handle();
	wsclient.connect(connection);
	}

void ComputeThread::run()
	{
	init();
	try_connect();

	// Enter run loop, which will never terminate anymore. The on_fail and on_close
	// handlers will re-try to establish connections when they go bad.

	wsclient.run();
	}

void ComputeThread::on_fail(websocketpp::connection_hdl hdl) 
	{
	connection_is_open=false;
	if(gui)
		gui->on_network_error();

	try_spawn_server();
	sleep(1);
	try_connect();
	}

void ComputeThread::try_spawn_server()
	{
	// Startup the server. First generate a UUID, pass this to the
	// starting server, then use this UUID to get access to the server
	// port.

	pid_t pid;
	char * const sargv[] = {"sh", "-c", "cadabra-server", NULL};
	int status;
	status = posix_spawn(&pid, "/bin/sh", NULL, NULL, sargv, environ);
	}

void ComputeThread::on_open(websocketpp::connection_hdl hdl) 
	{
	connection_is_open=true;
	if(gui)
		gui->on_connect();

//	// now it is safe to use the connection
//	std::string msg;
//
////	if(stopit) {
////		msg = 
////			"{ \"header\":   { \"uuid\": \"none\", \"msg_type\": \"execute_interrupt\" },"
////			"  \"content\":  { \"code\": \"print(42)\n\"} "
////			"}";
////		} 
////	else {
//		msg = 
//			"{ \"header\":   { \"uuid\": \"none\", \"msg_type\": \"execute_request\" },"
//			"  \"content\":  { \"code\": \"import time\nprint(42)\ntime.sleep(10)\n\"} "
//			"}";
////		}
//
////	c->send(hdl, "import time\nfor i in range(0,10):\n   print('this is python talking '+str(i))\nex=Ex('A_{m n}')\nprint(str(ex))", websocketpp::frame::opcode::text);
//	c->send(hdl, msg, websocketpp::frame::opcode::text);
	}

void ComputeThread::on_close(websocketpp::connection_hdl hdl) 
	{
	std::cerr << "connection closed" << std::endl;
	connection_is_open=false;
	if(gui)
		gui->on_disconnect();

	sleep(1);
	try_connect();
	}


void ComputeThread::on_message(websocketpp::connection_hdl hdl, message_ptr msg) 
	{
	client::connection_ptr con = wsclient.get_con_from_hdl(hdl);
	std::cerr << msg->get_payload() << std::endl;

	// Parse the JSON message.
	Json::Value  root;
	Json::Reader reader;
	bool success = reader.parse( msg->get_payload(), root );
	if ( !success ) {
		std::cerr << "cannot parse message" << std::endl;
		return;
		}
	const Json::Value header   = root["header"];
	const Json::Value content  = root["content"];
	const Json::Value msg_type = root["msg_type"];
	uint64_t id = header["cell_id"].asUInt64();

	// Stupid way to find cell by id.
	running_cells.erase(id);
	auto it=docthread.dtree().begin();
	while(it!=docthread.dtree().end()) {
		if((*it).id()==id)
			break;
		++it;
		}
	if(it==docthread.dtree().end()) {
		std::cerr << "uhoh, cannot find cell" << std::endl;
		return;
		}

	if(msg_type.asString()=="response") {
		std::string output = "$"+content["output"].asString()+"$";
		if(output!="$$") {

			// Stick an AddCell action onto the stack. We instruct the action to add this result output
			// cell as a child of the corresponding input cell.
			DataCell result(DataCell::CellType::output, output);
			
			// Finally, the actions. One to add the output cell, and another one to position the
			// cursor in the next input cell, creating one if it does not exist.
			std::shared_ptr<ActionBase> action = 
				std::make_shared<ActionAddCell>(result, it, ActionAddCell::Position::child);
			docthread.queue_action(action);
			}
		
		std::shared_ptr<ActionBase> actionpos =
			std::make_shared<ActionPositionCursor>(it, ActionPositionCursor::Position::next);
		docthread.queue_action(actionpos);
		}
	else {
		std::string error = "\\color{red}{\\begin{verbatim}"+content["error"].asString()+"\\end{verbatim}}";

		// Stick an AddCell action onto the stack. We instruct the action to add this result output
		// cell as a child of the corresponding input cell.
		DataCell result(DataCell::CellType::output, error);
		
		// Finally, the action.
		std::shared_ptr<ActionBase> action = 
			std::make_shared<ActionAddCell>(result, it, ActionAddCell::Position::child);
		docthread.queue_action(action);
		}


	gui->process_data();
	}

void ComputeThread::execute_cell(const DataCell& dc)
	{
	if(connection_is_open==false)
		return;

	std::cout << "going to execute " << dc.textbuf << std::endl;

	Json::Value req, header, content;
	header["uuid"]="none";
	header["cell_id"]=(Json::UInt64)dc.id();
	header["msg_type"]="execute_request";
	req["header"]=header;
	content["code"]=dc.textbuf;
	req["content"]=content;

	std::ostringstream str;
	str << req << std::endl;
	
	std::cerr << str.str() << std::endl;

	running_cells.insert(dc.id());
	wsclient.send(our_connection_hdl, str.str(), websocketpp::frame::opcode::text);
	}

int ComputeThread::number_of_cells_running() const
	{
	return running_cells.size();
	}

void ComputeThread::stop()
	{
	if(connection_is_open==false)
		return;

	Json::Value req, header, content;
	header["uuid"]="none";
	header["msg_type"]="execute_interrupt";
	req["header"]=header;

	std::ostringstream str;
	str << req << std::endl;
	
	std::cerr << str.str() << std::endl;

	wsclient.send(our_connection_hdl, str.str(), websocketpp::frame::opcode::text);
	}
