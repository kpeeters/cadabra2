
#pragma once

#include <string>

namespace cadabra {

	/// Return an absolute path to the installation path. This is
	/// determined at runtime, to allow for binary distributions to be
	/// installed at any location. Note that this cannot be used
	/// if the binary running the code was a python interpreter.

	std::string install_prefix();

	/// Just get a constant char array with the install prefix.

	const char *cmake_install_prefix();
	
	}
