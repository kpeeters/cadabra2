
#include "Config.hh"
#include "Snoop.hh"
#include "Server.hh"
#include <glibmm/miscutils.h>

#define NDEBUG 1

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

int main(int argc, char **argv)
	{
#ifndef ENABLE_JUPYTER
 	snoop::log.init("CadabraServer", CADABRA_VERSION_FULL, "log.cadabra.science");
 	snoop::log.set_sync_immediately(true);
#endif
	
#ifdef _WIN32
	snoop::log("platform") << "windows" << snoop::flush;
#else
#ifdef __APPLE__
	snoop::log("platform") << "macos" << snoop::flush;
#else
	snoop::log("platform") << "linux" << snoop::flush;
#endif
#endif
	
	int port=0;
	bool eod=true;
	if(argc>1)
		port=atoi(argv[1]);
	if(argc>2)
		eod=(atoi(argv[2])==1);

	Server server;
	server.run(port, eod);

	return 0;
	}


#if defined(_WIN32) && defined(NDEBUG)
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
	{
	FreeConsole();
	return main(__argc, __argv);
	}
#endif
