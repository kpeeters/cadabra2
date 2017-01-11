#include "cadabra2++/Parser.hh"
#include "cadabra2++/Storage.hh"
#include "cadabra2++/DisplayTerminal.hh"
#include "cadabra2++/algorithms/substitute.hh"
#include "cadabra2++/TerminalStream.hh"
#include "cadabra2++/properties/PartialDerivative.hh"

#include <iostream>
#include <sstream>

int main(int argc, char **argv)
	{
	cadabra::Kernel kernel;

	// The following few lines are equivalent to entering
	//
	//    {m,n,p,q}::Indices(position=free).
	//    \partial{#}::PartialDerivative;
	//    ex:= \int{ F_{m n} F^{m n} }{x};
	//    rl:= F_{m n} = \\partial_{m}{A_{n}} - \\partial_{n}{A_{m}};
	//    substitute(ex, rl, deep=True);
	//
	// in the Cadabra notebook.
	
	auto ind1 = kernel.ex_from_string("{m,n,p,q}");
	auto ind2 = kernel.ex_from_string("position=free");
	kernel.inject_property(new cadabra::Indices(), ind1, ind2);

	auto pd   = kernel.ex_from_string("\\partial{#}");
	kernel.inject_property(new cadabra::PartialDerivative(), pd, 0);

	auto ex = kernel.ex_from_string("\\int{ F_{m n} F^{m n} }{x}");
	auto rl = kernel.ex_from_string("F_{m n} = \\partial_{m}{A_{n}} - \\partial_{n}{A_{m}}");

	// Pretty-printing stream object.
	cadabra::TerminalStream ss(kernel, std::cerr);

	ss << ex << std::endl;
	ss << rl << std::endl;

	// Apply the 'substitute' algorithm.
	cadabra::substitute subs(kernel, *ex, *rl);
	auto top=ex->begin();
	subs.apply_generic(top, true, false, 0);

	ss << ex << std::endl;
	}
