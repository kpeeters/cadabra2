#pragma once

#include <iostream>
#include <variant>
#include <gmpxx.h>

namespace cadabra {

	class Multiplier {
		private:
			std::variant<mpq_class, double> value;

		public:
			// Constructors
			Multiplier();
			Multiplier(mpz_class numerator, mpz_class denominator=1);
			Multiplier(const mpq_class& v);
			Multiplier(int v);
			Multiplier(unsigned int v);
			Multiplier(long v);
			Multiplier(unsigned long v);
			Multiplier(double v);
    
			// Copy constructor
			Multiplier(const Multiplier& other) = default;
    
			// Assignment operators
			Multiplier& operator=(const Multiplier& other) = default;
			Multiplier& operator=(const mpq_class& v);
			Multiplier& operator=(const double& v);
    
			// Type checking
			bool is_rational() const;
			bool is_double() const;
    
			// Value getters
			const mpq_class& get_rational() const;
			double get_double() const;
    
			// Helper method to determine result type
			static bool result_is_double(const Multiplier& a, const Multiplier& b);

			// Canonicalize if rational.
			void canonicalize();
			
			// Arithmetic operators
			Multiplier operator+(const Multiplier& other) const;
			Multiplier operator-(const Multiplier& other) const;
			Multiplier operator*(const Multiplier& other) const;
			Multiplier operator/(const Multiplier& other) const;
    
			// Compound assignment operators
			Multiplier& operator+=(const Multiplier& other);
			Multiplier& operator-=(const Multiplier& other);
			Multiplier& operator*=(const Multiplier& other);
			Multiplier& operator/=(const Multiplier& other);
    
			// Comparison operators
			bool operator==(const Multiplier& other) const;
			bool operator!=(const Multiplier& other) const;
			bool operator<(const Multiplier& other) const;
			bool operator<=(const Multiplier& other) const;
			bool operator>(const Multiplier& other) const;
			bool operator>=(const Multiplier& other) const;
    
			// Stream output
			friend std::ostream& operator<<(std::ostream& os, const Multiplier& m);
	};

}
