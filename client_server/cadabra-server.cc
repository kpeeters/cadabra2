
#include "Config.hh"
#include "Snoop.hh"
#include "Server.hh"

#ifdef _WIN32
#include <Windows.h>
#endif

// Run a simple Cadabra server on a local port.

int main()
	{
#ifndef CONDA_FOUND
	snoop::log.init("CadabraServer", CADABRA_VERSION_FULL, "log.cadabra.science");
	snoop::log.set_sync_immediately(true);
#endif
	
	Server server;
	server.run();

//	snoop::log(snoop::info) << "Terminating" << snoop::flush;
#ifndef CONDA_FOUND
	snoop::log.sync_with_server();
#endif
	}


#if defined(_WIN32) && defined(NDEBUG)
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
	{
	main();
	}
#endif
