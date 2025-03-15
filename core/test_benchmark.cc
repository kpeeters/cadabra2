
#include "Storage.hh"
#include "NTensor.hh"
#include <iostream>

int main(int, char **)
	{
	std::cerr << "sizeof(str_node) = " << sizeof(cadabra::str_node) << std::endl;
	std::cerr << "sizeof(NTensor)  = " << sizeof(cadabra::NTensor) << std::endl;
	std::cerr << "sizeof(std::shared_ptr<NTensor>) = " << sizeof(std::shared_ptr<cadabra::NTensor>) << std::endl;
	std::cerr << "sizeof(nset_t::iterator)         = " << sizeof(cadabra::nset_t::iterator) << std::endl;
	std::cerr << "sizeof(multiplier_t)             = " << sizeof(cadabra::multiplier_t) << std::endl;
	std::cerr << "sizeof(tree_node_<str_node>)     = " << sizeof(tree_node_<cadabra::str_node>) << std::endl;
	}
