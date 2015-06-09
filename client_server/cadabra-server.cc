
#include "Server.hh"

// Run a simple Cadabra server on a local port.

int main()
	{
	std::cout << "Starting Cadabra server..." << std::endl;

	Server server;

	server.run();
	}
