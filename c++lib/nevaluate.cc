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

	auto ex = R"( A + B \cos( C ) )"_ex(k);
	std::cout << pprint(k, ex) << '\n';

	NTensor nt1({2,4});
	nt1.at({1,2}) = 3.1415;
	for(auto& v: nt1.values)
		std::cout << v << ", ";
	std::cout << "\n\n";
	std::cout << nt1 << std::endl;

	NTensor nt({2,4,3});
	nt.at({1,2,0}) = 6.2830;
	nt.at({0,3,2}) = -6.2830;
	std::cout << nt << std::endl;

	NEvaluator ev;
	ev.set_variable(Ex("C"), { 3.0 });
	ev.set_variable(Ex("B"), { 2.3 });
	ev.set_variable(Ex("A"), { 1.2 });
	double res = ev.evaluate(*ex);

	std::cout << res << std::endl;
	}
