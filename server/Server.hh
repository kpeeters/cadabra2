
#pragma once

// A Cadabra server object. An object of this type can run blocks of Python
// (or other language) code.
//
// Every block is run inside its own Python local scope. 
//
// The server uses a ... 0mq socket to listen for incoming requests.

#include <string>

class Server {
	public:
		Server();
		Server(const std::string& socket);

		void run();

	private:
		std::string socket_name;
};
