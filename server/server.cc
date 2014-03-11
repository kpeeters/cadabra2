
#include "Server.hh"

// Run a simple Cadabra server on a local port.

int main()
	{
	Server server("http://localhost:5555");

	server.run();
	}
