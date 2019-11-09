
#include "Config.hh"
#include "Snoop.hh"
#include "Server.hh"
#include <glibmm/miscutils.h>

#ifdef _WIN32
#include <Windows.h>
#include <WinReg.hpp>
#endif

// Run a simple Cadabra server on a local port.

int main()
	{
#ifdef _WIN32
	// The Anaconda people _really_ do not understand packaging...
	// We are going to find out the installation path for Anaconda/Miniconda
	// by querying a registry key.
	std::string pythonhome=Glib::getenv("PYTHONHOME");
	std::string pythonpath=Glib::getenv("PYTHONPATH");
	
	winreg::RegKey  key{ winreg::HKEY_CURRENT_USER, L"SOFTWARE\\Python\\PythonCore\\3.7" };
	std::wstring s  = key.GetStringValue(L"");
	
//	Glib::setenv("PYTHONHOME", (pythonhome.size()>0)?(pythonhome+":"):"" + Glib::get_home_dir()+"/Anaconda3");
//	Glib::setenv("PYTHONPATH", (pythonpath.size()>0)?(pythonpath+":"):"" + Glib::get_home_dir()+"/Anaconda3");
	Glib::setenv("PYTHONHOME", (pythonhome.size()>0)?(pythonhome+":"):"" + s);
	Glib::setenv("PYTHONPATH", (pythonpath.size()>0)?(pythonpath+":"):"" + s);
//	std::cerr << "Server::init: using PYTHONPATH = " << Glib::getenv("PYTHONPATH")
//				 << " and PYTHONHOME = " << Glib::getenv("PYTHONHOME") << "." << std::endl;
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
