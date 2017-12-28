#include "cadabra2++/Parser.hh"
#include "cadabra2++/Storage.hh"
#include "cadabra2++/DisplayTerminal.hh"
#include "cadabra2++/algorithms/substitute.hh"
#include "cadabra2++/algorithms/evaluate.hh"
#include "cadabra2++/TerminalStream.hh"
#include "cadabra2++/properties/PartialDerivative.hh"
#include "cadabra2++/properties/Coordinate.hh"

#include <iostream>
#include <sstream>

/// \file simple.cc
/// \ingroup libcadabra
///
/// Sample program to demonstrate the use of Cadabra directly from C++ code.

void test1()
	{
	// The following few lines are equivalent to entering
	//
	//    {r,t}::Coordinate.
	//    {m,n}::Indices(values={t,r}, position=free).
	//    ex:= A_{m} A^{m};
	//    rl:= A_{t} = 3 + a;
	//    evaluate(ex, rl);
	//
	// in the Cadabra notebook.

	cadabra::Kernel kernel;

	kernel.inject_property(new cadabra::Coordinate(), kernel.ex_from_string("{r,t}"), 0);
	kernel.inject_property(new cadabra::Indices(),    kernel.ex_from_string("{m,n}"),
								  kernel.ex_from_string("values={t,r}, position=free"));
	
	auto ex = kernel.ex_from_string("A_{m} A^{m}");
	auto rl = kernel.ex_from_string("A_{t} = 3 + a ");
	cadabra::evaluate ev(kernel, *ex, *rl);
	ev.apply_generic();

	// Pretty-printing stream object.
	cadabra::TerminalStream ss(kernel, std::cerr);
	ss << ex << std::endl;
	}

void test2()
	{
	// The following few lines are equivalent to entering
	//
	//    {m,n,p,q}::Indices(position=free).
	//    \partial{#}::PartialDerivative;
	//    ex:= \int{ F_{m n} F^{m n} }{x};
	//    rl:= F_{m n} = \\partial_{m}{A_{n}} - \\partial_{n}{A_{m}};
	//    substitute(ex, rl, deep=True);
	//
	// in the Cadabra notebook.

	cadabra::Kernel kernel;
	
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
	subs.apply_generic();

	ss << ex << std::endl;
	}


int main(int argc, char **argv)
	{
	test1();
	test2();
	}
