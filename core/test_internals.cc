
#include <catch2/catch_test_macros.hpp>

#include "Storage.hh"
#include "Compare.hh"

using namespace cadabra;

TEST_CASE( "Ex equality", "[ex1]" )
	{
	Ex ex1("y");
	Ex ex2("y");
	
	REQUIRE( ex1 == ex2 );
	}

TEST_CASE("Ex comparison", "[ex2]")
	{
	Ex ex1("y");
	Ex ex2("y");

	std::map<Ex, int, tree_exact_less_no_wildcards_obj> mp;
	mp[ex1]=3;
	mp[ex2]=4;

	REQUIRE( mp.size() == 1 );
	}  

//	std::cerr << "sizeof(ExNode) = " << sizeof(ExNode) << std::endl;

