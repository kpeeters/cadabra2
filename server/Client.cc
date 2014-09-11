
#include <string>
#include <iostream>
#include "Client.hh"

using namespace cadabra;

Client::Client()
	: wsclient(0)
	{
//	add_cell();
	}

Client::~Client()
	{
	if(wsclient)
		delete wsclient;
	}

void Client::run()
	{
	wsclient = new WSClient();

	wsclient->clear_access_channels(websocketpp::log::alevel::all);
	wsclient->clear_error_channels(websocketpp::log::elevel::all);

	std::string uri = "ws://localhost:9002";
	
	wsclient->init_asio();
	wsclient->set_open_handler(bind(&Client::on_open, this, wsclient, ::_1));
	wsclient->set_fail_handler(bind(&Client::on_fail, this, wsclient, ::_1));
	wsclient->set_close_handler(bind(&Client::on_close, this, wsclient, ::_1));
	wsclient->set_message_handler(bind(&Client::on_message, this, wsclient, ::_1, ::_2));
	
	websocketpp::lib::error_code ec;
	WSClient::connection_ptr con = wsclient->get_connection(uri, ec);
	wsclient->connect(con);

	// Start the ASIO io_service run loop
	wsclient->run();
	}

void Client::on_fail(WSClient* c, websocketpp::connection_hdl hdl) 
	{
	on_network_error();
	}

void Client::on_open(WSClient* c, websocketpp::connection_hdl hdl) 
	{
	our_connection_hdl = hdl;
	on_connect();

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

void Client::on_close(WSClient* c, websocketpp::connection_hdl hdl) 
	{
	on_disconnect();
	}

const Client::DTree& Client::dtree() 
	{
	return doc;
	}

void Client::on_message(WSClient* c, websocketpp::connection_hdl hdl, message_ptr msg) 
	{
	WSClient::connection_ptr con = c->get_con_from_hdl(hdl);
	
	std::cout << "received message on channel " << con->get_resource() << std::endl;
	std::cout << msg->get_payload() << std::endl;
	}


void Client::perform(const ActionBase& ab) 
	{
	// FIXME: this is just a test action
	std::string msg = 
		"{ \"header\":   { \"uuid\": \"none\", \"msg_type\": \"execute_request\" },"
		"  \"content\":  { \"code\": \"import time\nprint(42)\ntime.sleep(10)\n\"} "
		"}";
	wsclient->send(our_connection_hdl, msg, websocketpp::frame::opcode::text);
	}


Client::DataCell::DataCell(CellType t, const std::string& str, bool texhidden) 
	{
	cell_type = t;
	textbuf = str;
	tex_hidden = texhidden;
	}
