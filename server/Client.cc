
#include <string>
#include <iostream>

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/common/functional.hpp>

// Simple test program to talk to a cadabra server.

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
typedef websocketpp::client<websocketpp::config::asio_client> client;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

Client::Client()
	{
//	add_cell();
	}

void CadabraClient::run()
	{
	zmq::context_t context (1);
	zmq::socket_t socket (context, ZMQ_REQ);
	
	std::cout << "Connecting to hello world server…" << std::endl;
	socket.connect ("tcp://localhost:5555");
	
	//  Do 10 requests, waiting each time for a response
	for (int request_nbr = 0; request_nbr != 10; request_nbr++) {
		zmq::message_t request (6);
		memcpy ((void *) request.data (), "Hello", 5);
		std::cout << "Sending Hello " << request_nbr << "…" << std::endl;
		socket.send (request);
		
		//  Get the reply.
		zmq::message_t reply;
		socket.recv (&reply);
		std::cout << "Received World " << request_nbr << std::endl;
		}
	return 0;
	}
