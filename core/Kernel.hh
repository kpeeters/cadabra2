
#pragma once

#include "Props.hh"

/// \ingrou core
///
/// Cadabra kernel that keeps all state information that needs to be passed
/// around to algorithms and properties. At the moment only stores property
/// information, but could also store global settings and the like at some
/// later stage.

class Kernel {
	public:
		Kernel();
		Kernel(const Kernel& other);
		~Kernel();

		Properties properties;
		
};
