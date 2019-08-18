
#include "Config.hh"
#include "Snoop.hh"
#include "Server.hh"
#include <glibmm/miscutils.h>

#ifdef _WIN32
#include <Windows.h>
#endif

// Run a simple Cadabra server on a local port.

int main()
	{
#ifdef _WIN32
	// The Anaconda people _really_ do not understand packaging...
	std::string pythonhome=Glib::getenv("PYTHONHOME");
	std::string pythonpath=Glib::getenv("PYTHONPATH");	
	Glib::setenv("PYTHONHOME", (pythonhome.size()>0)?(pythonhome+":"):"" + Glib::get_home_dir()+"/Anaconda3");
	Glib::setenv("PYTHONPATH", (pythonpath.size()>0)?(pythonpath+":"):"" + Glib::get_home_dir()+"/Anaconda3");
	std::cerr << "Server::init: using PYTHONPATH = " << Glib::getenv("PYTHONPATH")
				 << " and PYTHONHOME = " << Glib::getenv("PYTHONHOME") << "." << std::endl;
#endif
	
#ifndef ENABLE_JUPYTER
	snoop::log.init("CadabraServer", CADABRA_VERSION_FULL, "log.cadabra.science");
	snoop::log.set_sync_immediately(true);
#endif

	Server server;
	server.run();

//	snoop::log(snoop::info) << "Terminating" << snoop::flush;
#ifndef ENABLE_JUPYTER
	snoop::log.sync_with_server();
#endif
	}


#if defined(_WIN32) && defined(NDEBUG)
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
	{
	main();
	}
#endif
