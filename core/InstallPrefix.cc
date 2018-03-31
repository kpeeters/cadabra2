
#include "Config.hh"
#include "InstallPrefix.hh"
#if !defined(__OpenBSD__)
  #include "whereami.h"
#endif
#include <stdexcept>
#include <stdlib.h>

std::string cadabra::install_prefix()
   {
#if defined(__FreeBSD__) || defined(__OpenBSD__)
   std::string ret(CMAKE_INSTALL_PREFIX);
   return ret;
#else
   std::string ret;
   int dirname_length;
   auto length = wai_getExecutablePath(NULL, 0, &dirname_length);
   if(length > 0) {
	   char *path = (char*)malloc(length + 1);
		if (!path)
			throw std::logic_error("Cannot determine installation path.");
		wai_getExecutablePath(path, length, &dirname_length);
		path[length] = '\0';
		path[dirname_length] = '\0';
		ret=std::string(path);
		free(path);
		ret=ret.substr(0, ret.size()-4); // strip '/bin'
		}
	return ret;
#endif
   }
	
