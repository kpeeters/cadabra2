
#pragma once

// A Cadabra server object. An object of this type can run blocks of Python
// (or other language) code.
//
// Every block is run inside its own Python local scope. 
//
// The server uses websocket++ sockets to listen for incoming requests.
// For testing purposes, look at test_server.cc and test_talk_to_server.cc.

#include <string>
#include <boost/python.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/common/functional.hpp>

class Server {
	public:
		Server();
		Server(const std::string& socket);

		void run();

	private:
		typedef websocketpp::server<websocketpp::config::asio> server;
		server print_server;
		std::string socket_name;

		void setup_python();

		void on_socket_init(websocketpp::connection_hdl hdl, boost::asio::ip::tcp::socket & s);
		void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg);

		boost::python::object main_module;
		boost::python::object main_namespace;
};
