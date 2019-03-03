
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/common/functional.hpp>

// Simple test program to talk to a cadabra server.

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
typedef websocketpp::client<websocketpp::config::asio_client> client;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

bool stopit=false;

void on_open(client* c, websocketpp::connection_hdl hdl)
	{
	// now it is safe to use the connection
	std::cout << "connection ready" << std::endl;

	std::string msg;

	if(stopit) {
		msg =
		   "{ \"header\":   { \"uuid\": \"none\", \"msg_type\": \"execute_interrupt\" },"
		   "  \"content\":  { \"code\": \"print(42)\n\"} "
		   "}";
		}
	else {
		msg =
		   "{ \"header\":   { \"uuid\": \"none\", \"msg_type\": \"execute_request\" },"
		   "  \"content\":  { \"code\": \"import time\nprint(42)\ntime.sleep(10)\n\"} "
		   "}";
		}

	//	c->send(hdl, "import time\nfor i in range(0,10):\n   print('this is python talking '+str(i))\nex=Ex('A_{m n}')\nprint(str(ex))", websocketpp::frame::opcode::text);
	c->send(hdl, msg, websocketpp::frame::opcode::text);
	}

void on_message(client* c, websocketpp::connection_hdl hdl, message_ptr msg)
	{
	client::connection_ptr con = c->get_con_from_hdl(hdl);

	std::cout << "received message on channel " << con->get_resource() << std::endl;
	std::cout << msg->get_payload() << std::endl;
	}


int main(int argc, char **argv)
	{
	if(argc>1) stopit=true;

	client c;
	c.clear_access_channels(websocketpp::log::alevel::all);
	c.clear_error_channels(websocketpp::log::elevel::all);

	std::string uri = "ws://localhost:9002";

	c.init_asio();
	c.set_open_handler(bind(&on_open,&c,::_1));
	c.set_message_handler(bind(&on_message,&c,::_1,::_2));

	websocketpp::lib::error_code ec;
	client::connection_ptr con = c.get_connection(uri, ec);
	c.connect(con);

	std::cout << "connected" << std::endl;

	// Start the ASIO io_service run loop
	c.run();

	std::cout << "run loop terminated" << std::endl;
	}
