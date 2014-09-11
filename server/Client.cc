
#include <string>
#include <iostream>
#include "Client.hh"

using namespace cadabra;

Client::Client()
	{
//	add_cell();
	}

void Client::run()
	{
	wsclient c;
	c.clear_access_channels(websocketpp::log::alevel::all);
	c.clear_error_channels(websocketpp::log::elevel::all);

	std::string uri = "ws://localhost:9002";
	
	c.init_asio();
	c.set_open_handler(bind(&Client::on_open, this, &c, ::_1));
	c.set_fail_handler(bind(&Client::on_fail, this, &c, ::_1));
	c.set_close_handler(bind(&Client::on_close, this, &c, ::_1));
	c.set_message_handler(bind(&Client::on_message, this, &c, ::_1, ::_2));
	
	websocketpp::lib::error_code ec;
	wsclient::connection_ptr con = c.get_connection(uri, ec);
	c.connect(con);

	// Start the ASIO io_service run loop
	c.run();
	}

void Client::on_fail(wsclient* c, websocketpp::connection_hdl hdl) 
	{
	on_network_error();
	}

void Client::on_open(wsclient* c, websocketpp::connection_hdl hdl) 
	{
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

void Client::on_close(wsclient* c, websocketpp::connection_hdl hdl) 
	{
	on_disconnect();
	}

const Client::DTree& Client::dtree() 
	{
	return doc;
	}

void Client::on_message(wsclient* c, websocketpp::connection_hdl hdl, message_ptr msg) 
	{
	wsclient::connection_ptr con = c->get_con_from_hdl(hdl);
	
	std::cout << "received message on channel " << con->get_resource() << std::endl;
	std::cout << msg->get_payload() << std::endl;
	}



Client::DataCell::DataCell(CellType t, const std::string& str, bool texhidden) 
	{
	cell_type = t;
	textbuf = str;
	tex_hidden = texhidden;
	}
