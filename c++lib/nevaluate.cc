#include "cadabra2++.hh"
#include <iostream>

using namespace cadabra;
using namespace cadabra::cpplib;

int main(int, char **)
	{
	Kernel k(true);
	pprint_enable_utf8();
	//{\mu,\nu}::Indices(vector).
	// tr{#}::Trace.
	// u^{\mu}::SelfNonCommuting.
	// u^{\mu}::ImplicitIndex.
	// ex:=tr{A u^{\nu} u^{\mu} u^{\mu} u^{\nu} + B u^{\mu} u^{\mu} u^{\nu}
	// u^{\nu}}: meld(_);

//	inject_property<SelfNonCommuting>(k, "{A,B,C,D }");
//	inject_property<Trace>(k, "tr{#}");

	// Multiplying two scalar variables which each take
	// an array of values leads to an outer product.

	auto ex1b = "B*A + A"_ex(k);
	NEvaluator ev1b;
	ev1b.set_variable(Ex("A"), NTensor({1.0, 2.0, 3.0}));
	ev1b.set_variable(Ex("B"), NTensor({0.5, 1.0, 5.0, 10.0}));
	// This should give a {3, 4} tensor.
	auto res1b = ev1b.evaluate(*ex1b);
	std::cout << res1b << "\n\n";

	auto ex1 = "B*A + C"_ex(k);
	NEvaluator ev1;
	ev1.set_variable(Ex("A"), NTensor({1.0, 2.0, 3.0}));
	ev1.set_variable(Ex("B"), NTensor({0.5, 1.0, 5.0, 10.0}));
	ev1.set_variable(Ex("C"), NTensor({1.0, -1.0}));
	// This should give a {3, 4, 2} tensor.
	auto res1 = ev1.evaluate(*ex1);
	std::cout << res1 << "\n\n";

	// Trigonometric functions.

	auto ex2 = R"( A + B \cos( C ) )"_ex(k);
	std::cout << pprint(k, ex2) << '\n';
	NTensor nt2({2,4}, 0.0);
	nt2.at({1,2}) = 3.1415;
	for(auto& v: nt2.values)
		std::cout << v << ", ";
	std::cout << "\n\n";
	std::cout << nt2 << std::endl;

	NEvaluator ev;
	ev.set_variable(Ex("C"), { 3.0 });
	ev.set_variable(Ex("B"), { 2.3 });
	ev.set_variable(Ex("A"), { 1.2 });
	auto res2 = ev.evaluate(*ex2);
	std::cout << res2 << std::endl;

	// Double trig.
	auto ex3 = R"( \cos(x) \sin(y) )"_ex(k);
	NEvaluator ev3;
	ev3.set_variable(Ex("x"), NTensor::linspace(0.0, 3.14, 20));
	ev3.set_variable(Ex("y"), NTensor::linspace(0.0, 3.14, 20));
	auto res3 = ev3.evaluate(*ex3);
	std::cout << res3 << std::endl;

	// Array indexing.

	NTensor nt({2,4,3}, 0.0);
	nt.at({1,2,0}) = 6.2830;
	nt.at({0,3,2}) = -6.2830;
	std::cout << nt << std::endl;


	}
