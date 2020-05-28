#include "cadabra2++.hh"
#include <iostream>

using namespace cadabra;

int main(int, char **)
	{
	Kernel kernel;
	
	Ex ex("X_{a} dX_{b} B^{a b}");// + X_{b} C^{b}");
	IndexMap im;
	
	AdjformEx adex( ex.begin(), im, kernel );

	TerminalStream ss(kernel, std::cerr);
	ss << ex << std::endl;
	ss << adex << std::endl;
	
	}
