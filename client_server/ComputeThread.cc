
#include <string>
#include <iostream>
#include <sstream>
#include "ComputeThread.hh"
#include "DocumentThread.hh"
#include "GUIBase.hh"
#include "Actions.hh"

using namespace cadabra;
typedef websocketpp::client<websocketpp::config::asio_client> client;

ComputeThread::ComputeThread(GUIBase *g, DocumentThread& dt)
	: gui(g), docthread(dt)
	{
	}

ComputeThread::~ComputeThread()
	{
	}

void ComputeThread::init() 
	{
	// Setup the WebSockets client.
	wsclient.clear_access_channels(websocketpp::log::alevel::all);
	wsclient.clear_error_channels(websocketpp::log::elevel::all);

	wsclient.init_asio();

	using websocketpp::lib::placeholders::_1;
	using websocketpp::lib::placeholders::_2;
	using websocketpp::lib::bind;
	wsclient.set_open_handler(bind(&ComputeThread::on_open, this, ::_1));
	wsclient.set_fail_handler(bind(&ComputeThread::on_fail, this, ::_1));
	wsclient.set_close_handler(bind(&ComputeThread::on_close, this, ::_1));
	wsclient.set_message_handler(bind(&ComputeThread::on_message, this, ::_1, ::_2));
	std::string uri = "ws://localhost:9002";
	
	websocketpp::lib::error_code ec;
	WSClient::connection_ptr con = wsclient.get_connection(uri, ec);
	if (ec) {
		std::cout << "error: " << ec.message() << std::endl;
		return;
		}

	our_connection_hdl = con->get_handle();
	wsclient.connect(con);
	}

void ComputeThread::run()
	{
	init();

	// Start the ASIO io_service run loop
	wsclient.run();
	}

void ComputeThread::on_fail(websocketpp::connection_hdl hdl) 
	{
	if(gui)
		gui->on_network_error();
	}

void ComputeThread::on_open(websocketpp::connection_hdl hdl) 
	{
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
	if(gui)
		gui->on_disconnect();
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
	const Json::Value header  = root["header"];
	const Json::Value content = root["content"];
	uint64_t id = header["cell_id"].asUInt64();
	std::string output = content["output"].asString();

	// Stick an AddCell action onto the stack. We instruct the action to add this result output
	// cell as a child of the corresponding input cell.
	DataCell result(DataCell::CellType::output, output);

	// Stupid way to find cell by id.
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

	// Finally, the action.
	std::shared_ptr<ActionBase> action = 
		std::make_shared<ActionAddCell>(result, it, ActionAddCell::Position::child);

	docthread.queue_action(action);
	gui->process_data();
	}

void ComputeThread::execute_cell(const DataCell& dc)
	{
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

	wsclient.send(our_connection_hdl, str.str(), websocketpp::frame::opcode::text);
	}
