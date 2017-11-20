
#include "MultiIndex.hh"

#include <string>
#include <iostream>

int main()
	{

	MultiIndex<std::string> mi;
	mi.values.push_back( { "a", "b", "c" } );
	mi.values.push_back( { "0", "1", "2", "3" } );
	mi.values.push_back( { "q", "r" } );	

	for(mi.start(); !mi.end(); ++mi) {
		for(std::size_t i=0; i<mi.values.size(); ++i)
			std::cerr << mi[i] << " ";
		std::cerr << std::endl;
		}
	}
