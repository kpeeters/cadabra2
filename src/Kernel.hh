
#pragma once

#include "Props.hh"

// Cadabra kernel that keeps all global state information.

class Kernel {
	public:
		Kernel();
		Kernel(const Kernel& other);

		Properties properties;
		
};
