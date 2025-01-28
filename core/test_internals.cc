
#include "Storage.hh"
#include "Compare.hh"
#include "NTensor.hh"
#include <iostream>
#include <sstream>

int main(int argc, char **argv)
	{
	using namespace cadabra;

	Ex ex1("y");
	Ex ex2("y");

	std::cerr << (ex1==ex2) << std::endl;

	std::map<Ex, int, tree_exact_less_no_wildcards_obj> mp;
	mp[ex1]=3;
	mp[ex2]=4;

	std::cerr << mp.size() << std::endl;
	}
