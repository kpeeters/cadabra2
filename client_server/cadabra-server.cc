
#include "Config.hh"
#include "Snoop.hh"
#include "Server.hh"
#include <glibmm/miscutils.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef _WIN32

std::string getRegKey(const std::string& location, const std::string& name, bool system)
	{
	HKEY key;
	TCHAR value[1024]; 
	DWORD bufLen = 1024*sizeof(TCHAR);
	long ret;
	ret = RegOpenKeyExA(system?HKEY_LOCAL_MACHINE:HKEY_CURRENT_USER, location.c_str(), 0, KEY_QUERY_VALUE, &key);
	if( ret != ERROR_SUCCESS ){
		return std::string();
		}
	ret = RegQueryValueExA(key, name.c_str(), 0, 0, (LPBYTE) value, &bufLen);
	RegCloseKey(key);
	if ( (ret != ERROR_SUCCESS) || (bufLen > 1024*sizeof(TCHAR)) ){
		return std::string();
		}
	std::string stringValue = std::string(value, (size_t)bufLen - 1);
	size_t i = stringValue.length();
	while( i > 0 && stringValue[i-1] == '\0' ){
		--i;
		}
	return stringValue.substr(0,i); 
	}

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

	std::string s = getRegKey("SOFTWARE\\Python\\PythonCore\\3.7", "", false);
	if(s=="")
		s = getRegKey("SOFTWARE\\Python\\PythonCore\\3.7", "", true);
	
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
