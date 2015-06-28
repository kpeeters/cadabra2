
#include "Server.hh"

// Run a simple Cadabra server on a local port.

int main()
	{
	std::cerr << "cadabra-server: starting" << std::endl;

	Server server;
	server.run();

	std::cerr << "cadabra-server: terminating" << std::endl;
	}
