#include "cadabra2++.hh"
#include <iostream>

using namespace cadabra;
using namespace cadabra::cpplib;

int main(int, char**)
{
	Kernel k(true);
	pprint_enable_utf8();
    //{\mu,\nu}::Indices(vector).
    //tr{#}::Trace.
    //u^{\mu}::SelfNonCommuting.
    //u^{\mu}::ImplicitIndex.
    //ex:=tr{A u^{\nu} u^{\mu} u^{\mu} u^{\nu} + B u^{\mu} u^{\mu} u^{\nu} u^{\nu}}:
    //meld(_);

	inject_property<SelfNonCommuting>(k, "{A,B,C,D }");
	inject_property<Trace>(k, "tr{#}");

	auto ex = R"(tr(A B C D + B C D A)"_ex(k);
	meld m(k, *ex);
	std::cout << pprint(k, ex) << '\n';
	m.apply_pre_order();
	std::cout << pprint(k, ex) << '\n';
	// assert ex == $2 * Tr{ A B C D }$
	
}
