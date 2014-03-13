
#include "Kernel.hh"

Kernel::Kernel()
	{
	}

Kernel::Kernel(const Kernel& other)
	{
	std::cout << "KERNEL COPY " << &other << " -> " << this << std::endl;
	properties=other.properties;
	}
