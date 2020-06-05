#include "cadabra2++.hh"
#include <iostream>

using namespace cadabra;

int main(int, char **)
	{
	Kernel kernel;
	
	auto ex1 = kernel.ex_from_string("X_{a} dX_{b} B^{a b}");
	auto ex2 = kernel.ex_from_string("X_{b} dX_{a} C^{b a}");	
	IndexMap im;
	
	AdjformEx adex1( ex1->begin(), im, kernel );
	AdjformEx adex2( ex2->begin(), im, kernel );

	std::cerr << adex1.compare(adex2) << std::endl;

	TerminalStream ss(kernel, std::cerr);
	ss << ex1 << std::endl;
	ss << ex2 << std::endl;
	ss << adex1 << std::endl;
	ss << adex2 << std::endl;
	
	}
