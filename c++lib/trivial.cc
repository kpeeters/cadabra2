#include "cadabra2++.hh"
#include <iostream>

using namespace cadabra;
using namespace cadabra::cpplib;

int main() {
   Kernel k(true);
   inject_property<AntiCommuting>(k, "{A,B}");
   auto ex = "A B - B A"_ex(k);
   sort_product sp(k, *ex);
   sp.apply_generic();

	collect_terms ct(k, *ex);
	ct.apply_generic();
	
   std::cout << pprint(k, ex) << std::endl;
}
