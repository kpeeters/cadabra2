#include "Multiplier.hh"
#include <exception>

namespace cadabra {

	Multiplier::Multiplier()
		: value(mpq_class(0))
		{
		}

	Multiplier::Multiplier(mpz_class n, mpz_class d)
		: value(mpq_class(n, d))
		{
		}
	
	Multiplier::Multiplier(const mpq_class& v)
		: value(v)
		{
		}

	Multiplier::Multiplier(int v)
		: value(mpq_class(v))
		{
		}

	Multiplier::Multiplier(unsigned int v)
		: value(mpq_class(v))
		{
		}

	Multiplier::Multiplier(long v)
		: value(mpq_class(v))
		{
		}

	Multiplier::Multiplier(unsigned long v)
		: value(mpq_class(v))
		{
		}

	Multiplier::Multiplier(double v)
		: value(v)
		{
		}

	Multiplier& Multiplier::operator=(const mpq_class& v)
		{
		value = v;
		return *this;
		}

	Multiplier& Multiplier::operator=(const double& v)
		{
		value = v;
		return *this;
		}

	bool Multiplier::is_rational() const
		{
		return std::holds_alternative<mpq_class>(value);
		}

	bool Multiplier::is_double() const
		{
		return std::holds_alternative<double>(value);
		}

	const mpq_class& Multiplier::get_rational() const
		{
		if (is_rational()) {
			return std::get<mpq_class>(value);
			}
		else throw std::logic_error("Multiplier::get_rational: cannot convert double to rational.");
		}

	double Multiplier::get_double() const
		{
		if (is_double()) {
			return std::get<double>(value);
			}
		else {
			return std::get<mpq_class>(value).get_d();
			}
		}
	
	bool Multiplier::result_is_double(const Multiplier& a, const Multiplier& b)
		{
		return a.is_double() || b.is_double();
		}

	void Multiplier::canonicalize()
		{
		if(is_rational()) {
			mpq_class rat = get_rational();
			rat.canonicalize();
			value = rat;
			}
		}
	
	Multiplier Multiplier::operator+(const Multiplier& other) const
		{
		if (result_is_double(*this, other)) {
			return Multiplier(this->get_double() + other.get_double());
			}
		else {
			return Multiplier(this->get_rational() + other.get_rational());
			}
		}

	Multiplier Multiplier::operator-(const Multiplier& other) const
		{
		if (result_is_double(*this, other)) {
			return Multiplier(this->get_double() - other.get_double());
			}
		else {
			return Multiplier(this->get_rational() - other.get_rational());
			}
		}

	Multiplier Multiplier::operator*(const Multiplier& other) const
		{
		if (result_is_double(*this, other)) {
			return Multiplier(this->get_double() * other.get_double());
			}
		else {
			return Multiplier(this->get_rational() * other.get_rational());
			}
		}
	
	Multiplier Multiplier::operator/(const Multiplier& other) const
		{
		if (result_is_double(*this, other)) {
			return Multiplier(this->get_double() / other.get_double());
			}
		else {
			return Multiplier(this->get_rational() / other.get_rational());
			}
		}
	
	Multiplier& Multiplier::operator+=(const Multiplier& other)
		{
		*this = *this + other;
		return *this;
		}

	Multiplier& Multiplier::operator-=(const Multiplier& other)
		{
		*this = *this - other;
		return *this;
		}

	Multiplier& Multiplier::operator*=(const Multiplier& other)
		{
		*this = *this * other;
		return *this;
		}

	Multiplier& Multiplier::operator/=(const Multiplier& other)
		{
		*this = *this / other;
		return *this;
		}

	bool Multiplier::operator==(const Multiplier& other) const
		{
		if (is_rational() && other.is_rational()) {
			return get_rational() == other.get_rational();
			} else {
			return get_double() == other.get_double();
			}
		}

	bool Multiplier::operator!=(const Multiplier& other) const
		{
		return !(*this == other);
		}
	
	bool Multiplier::operator<(const Multiplier& other) const
		{
		if(is_rational() && other.is_rational()) {
			return get_rational() < other.get_rational();
			}
		else {
			return get_double() < other.get_double();
			}
		}

	bool Multiplier::operator<=(const Multiplier& other) const
		{
		if (is_rational() && other.is_rational()) {
			return get_rational() <= other.get_rational();
			}
		else {
			return get_double() <= other.get_double();
			}
		}
	
	bool Multiplier::operator>(const Multiplier& other) const
		{
		return !(*this <= other);
		}

	bool Multiplier::operator>=(const Multiplier& other) const
		{
		return !(*this < other);
		}

	std::ostream& operator<<(std::ostream& os, const Multiplier& m)
		{
		if (m.is_rational()) {
			os << std::get<mpq_class>(m.value);
			}
		else {
			os << std::get<double>(m.value);
			}
		return os;
		}
}
