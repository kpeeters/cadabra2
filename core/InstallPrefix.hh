
#pragma once

#include <string>

namespace cadabra {

	/// Return an absolute path to the installation path. This is
	/// determined at runtime, to allow for binary distributions to be
	/// installed at any location.

	std::string install_prefix();

	}
