#include <iostream>
#include "Multiplier.hh"

using namespace cadabra;

int main() {
    // Rational values
    mpq_class r1(1, 2); // 1/2
    mpq_class r2(3, 4); // 3/4
    
    Multiplier m1(r1);
    Multiplier m2(r2);
    Multiplier m3(0.5);  // double
    
    std::cout << "m1: " << m1 << std::endl;
    std::cout << "m2: " << m2 << std::endl;
    std::cout << "m3: " << m3 << std::endl;
    
    // Operations with rational values
    Multiplier sum_rational = m1 + m2;
    std::cout << "m1 + m2: " << sum_rational << " (type: " 
              << (sum_rational.is_rational() ? "rational" : "double") << ")" << std::endl;
    
    // Operations involving a double should convert to double
    Multiplier sum_mixed = m1 + m3;
    std::cout << "m1 + m3: " << sum_mixed << " (type: " 
              << (sum_mixed.is_rational() ? "rational" : "double") << ")" << std::endl;
    
    Multiplier product = m1 * m2;
    std::cout << "m1 * m2: " << product << " (type: " 
              << (product.is_rational() ? "rational" : "double") << ")" << std::endl;
    
    Multiplier quotient = m1 / m3;
    std::cout << "m1 / m3: " << quotient << " (type: " 
              << (quotient.is_rational() ? "rational" : "double") << ")" << std::endl;
    
    return 0;
}
