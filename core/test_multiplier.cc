#include <catch2/catch_test_macros.hpp>

#include <iostream>
#include "Multiplier.hh"

using namespace cadabra;

TEST_CASE("Multipliers", "[multiplier]")
	{
	// Rational values
	mpq_class r1(1, 2); // 1/2
	mpq_class r2(3, 4); // 3/4
   
	Multiplier m1(r1);
	Multiplier m2(r2);
	Multiplier m3(0.5);  // double
   
	Multiplier sum_rational = m1 + m2;
	Multiplier sum_mixed = m1 + m3;
	Multiplier product = m1 * m2;
	Multiplier quotient = m1 / m3;
	
	// Operations with rational values
	REQUIRE( sum_rational.is_rational() == true );
	REQUIRE( sum_rational.get_rational() == mpq_class(5, 4) );
	
	// Operations involving a double should convert to double
	REQUIRE( sum_mixed.is_rational() == false );
	REQUIRE( std::abs( sum_mixed.get_double() - 1 ) < 1e-9 );
	REQUIRE( product.is_rational() == true );
	REQUIRE( product.get_rational() == mpq_class(3, 8) );
	REQUIRE( quotient.is_rational() == false );
	REQUIRE( std::abs( quotient.get_double() - 1 ) < 1e-9 );
	}
