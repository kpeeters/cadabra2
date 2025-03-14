
#include "Storage.hh"
#include "NTensor.hh"
#include <iostream>

int main(int, char **)
	{
	std::cerr << "sizeof(str_node) = " << sizeof(cadabra::str_node) << std::endl;
	std::cerr << "sizeof(NTensor)  = " << sizeof(cadabra::NTensor) << std::endl;
	}
