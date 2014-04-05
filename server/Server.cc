

#include <iostream>

#include "Server.hh"

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

Server::Server()
	{
	socket_name="tcp://*:5454";
	setup_python();
	}

Server::Server(const std::string& socket)
	{
	socket_name=socket;
	setup_python();
	}

void Server::setup_python()
	{
	Py_Initialize();

	main_module = boost::python::import("__main__");
	main_namespace = main_module.attr("__dict__");

	boost::python::object ignored = boost::python::exec("hello = file('hello.txt', 'w')\n"
								 "hello.write('hello world')\n"
								 "hello.close()",
								 main_namespace);
	}

void Server::on_socket_init(websocketpp::connection_hdl hdl, boost::asio::ip::tcp::socket & s) 
	{
	boost::asio::ip::tcp::no_delay option(true);
	s.set_option(option);
	}

void Server::on_message(websocketpp::connection_hdl hdl, server::message_ptr msg) 
	{
	std::cout << "Message: " << msg->get_payload() << std::endl;
	print_server.send(hdl, "welcome", websocketpp::frame::opcode::text);
	}

void Server::run() 
	{
	print_server.set_socket_init_handler(bind(&Server::on_socket_init, this, ::_1,::_2));
	print_server.set_message_handler(bind(&Server::on_message, this, ::_1, ::_2));
	
	print_server.init_asio();
	print_server.set_reuse_addr(true);
	print_server.listen(9002);
	print_server.start_accept();
	
	print_server.run();
	}
