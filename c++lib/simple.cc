#include "cadabra2++/Parser.hh"
#include "cadabra2++/Storage.hh"
#include "cadabra2++/DisplayTerminal.hh"
#include "cadabra2++/algorithms/substitute.hh"

#include <sstream>

int main(int argc, char **argv)
	{
	Kernel kernel;

	auto ex = std::make_shared<Ex>();
	Parser parser1(ex, "\\int{ F_{m n} F^{m n} }{x}");
	auto rl = std::make_shared<Ex>();
	Parser parser2(rl, "F_{m n} = \\partial_{m}{A_{n}} - \\partial_{n}{A_{m}}");

	DisplayTerminal dt1(kernel, *ex);
	dt1.output(std::cerr);
	std::cerr << std::endl;
	DisplayTerminal dt2(kernel, *rl);
	dt2.output(std::cerr);
	std::cerr << std::endl;
	
	substitute subs(kernel, *ex, *rl);
	DisplayTerminal dt3(kernel, *ex);
	dt3.output(std::cerr);
	std::cerr << std::endl;
	}
