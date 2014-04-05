
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/common/functional.hpp>

// Simple test program to talk to a cadabra server.

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
typedef websocketpp::client<websocketpp::config::asio_client> client;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

int case_count = 0;

void on_open(client* c, websocketpp::connection_hdl hdl) 
	{
	// now it is safe to use the connection
	std::cout << "connection ready" << std::endl;
	c->send(hdl, "hi there", websocketpp::frame::opcode::text);
	}

void on_message(client* c, websocketpp::connection_hdl hdl, message_ptr msg) 
	{
	client::connection_ptr con = c->get_con_from_hdl(hdl);
	
	std::cout << "received message " << con->get_resource() << std::endl;

	if (con->get_resource() == "/getCaseCount") {
		std::cout << "Detected " << msg->get_payload() << " test cases."
					 << std::endl;
		case_count = atoi(msg->get_payload().c_str());
		} else {
		c->send(hdl, msg->get_payload(), msg->get_opcode());
		}
	}


int main()
	{
	client c;

	std::string uri = "ws://localhost:9002";

	c.init_asio();
	c.set_open_handler(bind(&on_open,&c,::_1));
	c.set_message_handler(bind(&on_message,&c,::_1,::_2));

	websocketpp::lib::error_code ec;
	client::connection_ptr con = c.get_connection(uri+"/getCaseCount", ec);
	c.connect(con);

	std::cout << "connected" << std::endl;
	
	// Start the ASIO io_service run loop
	c.run();

	std::cout << "running" << std::endl;
	
	std::cout << "case count: " << case_count << std::endl;
	
	for (int i = 1; i <= case_count; i++) {
		c.reset();
		
		std::stringstream url;
		
		url << uri << "/runCase?case=" << i << "&agent="
			 << websocketpp::user_agent;
		
		con = c.get_connection(url.str(), ec);
		
		c.connect(con);
		
		c.run();
		}
	
	}
