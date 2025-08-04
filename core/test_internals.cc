
#include <catch2/catch_test_macros.hpp>

#include "Storage.hh"
#include "Compare.hh"
#include "Grouping.hh"
#include "Kernel.hh"

using namespace cadabra;

TEST_CASE( "C++: Ex equality", "[ex1]" )
	{
	Ex ex1("y");
	Ex ex2("y");
	
	REQUIRE( ex1 == ex2 );
	}

TEST_CASE("C++: Ex comparison", "[ex2]")
	{
	Ex ex1("y");
	Ex ex2("y");

	std::map<Ex, int, tree_exact_less_no_wildcards_obj> mp;
	mp[ex1]=3;
	mp[ex2]=4;

	REQUIRE( mp.size() == 1 );
	}

TEST_CASE("C++: Ex grouping", "[group1]")
	{
	Kernel kernel(true);
	auto ex  = kernel.ex_from_string("{ A_{m n}, B_{m n}, A_{m n}, C_{m n}, B_{m n} }");
	auto grp = group_by_equivalence(*ex, ex->begin());
	
	REQUIRE( grp.size() == 2 );
	REQUIRE( grp[ex->child(ex->begin(), 2)].second == ex->child(ex->begin(), 0) );
	REQUIRE( grp[ex->child(ex->begin(), 4)].second == ex->child(ex->begin(), 1) );
	}

TEST_CASE("C++: advanced Ex grouping", "[group2]")
	{
	Kernel kernel(true);

	equiv_fun_t component_equiv = [](const Ex& ex, Ex::iterator it1, Ex::iterator it2) {
		it1 = ex.child(it1, 1);
		it2 = ex.child(it2, 1);
		return ex.equal_subtree(it1, it2);
		};
	
	auto ex  = kernel.ex_from_string("{ [1, 2] = A_{m n}, [1, 3] = B_{m n}, [1, 4] = A_{m n}, [2, 3] = C_{m n}, [2, 4] = B_{m n} }");
	auto grp2 = group_by_equivalence(*ex, ex->begin(), component_equiv);
	
	REQUIRE( grp2.size() == 2 );
	REQUIRE( grp2[ex->child(ex->begin(), 2)].second == ex->child(ex->begin(), 0) );
	REQUIRE( grp2[ex->child(ex->begin(), 4)].second == ex->child(ex->begin(), 1) );
	}

