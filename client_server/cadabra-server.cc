
#include "Config.hh"
#include "Snoop.hh"
#include "Server.hh"

#ifdef _WIN32
#include <Windows.h>
#endif

// Run a simple Cadabra server on a local port.

int main()
	{
	snoop::log.init("CadabraServer", CADABRA_VERSION_FULL, "log.cadabra.science");
	snoop::log.set_sync_immediately(true);
	//	snoop::log(snoop::info) << "Starting" << snoop::flush;

	Server server;
	server.run();

	//	snoop::log(snoop::info) << "Terminating" << snoop::flush;
	snoop::log.sync_with_server();
	}


#if defined(_WIN32) && defined(NDEBUG)
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
	{
	main();
	}
#endif