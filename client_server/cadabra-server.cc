
#include "Snoop.hh"
#include "Server.hh"

// Run a simple Cadabra server on a local port.

int main()
	{
	snoop::log.init("CadabraServer", "2.0", "/tmp/cadabra_server.sql", "http://log.cadabra.science");
	snoop::log.set_sync_immediately(true);
	snoop::log(snoop::info) << "Starting" << snoop::flush;

	Server server;
	server.run();

	snoop::log(snoop::info) << "Terminating" << snoop::flush;
	snoop::log.sync_with_server();
	}
