
#include <string>
#include <iostream>
#include "ComputeThread.hh"
#include "GUIBase.hh"

using namespace cadabra;

ComputeThread::ComputeThread(GUIBase *g)
	: gui(g)
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
//	wsclient.set_message_handler(bind(&ComputeThread::on_message, this, ::_1, ::_2));
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
//	WSComputeThread::connection_ptr con = c->get_con_from_hdl(hdl);
	
//	std::cout << "received message on channel " << con->get_resource() << std::endl;
//	std::cout << msg->get_payload() << std::endl;
	}

void ComputeThread::execute_cell(std::shared_ptr<DataCell>) 
	{
	
	}
