
#include "Kernel.hh"

Kernel::Kernel()
	{
	std::cerr << "Kernel() " << this << std::endl;
	}

Kernel::~Kernel()
	{
	std::cerr << "~Kernel() " << this << std::endl;
	}

Kernel::Kernel(const Kernel& other)
	{
	std::cerr << "KERNEL COPY " << &other << " -> " << this << std::endl;
	properties=other.properties;
	}
