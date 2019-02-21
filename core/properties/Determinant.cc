
#include "Exceptions.hh"
#include "properties/Determinant.hh"

using namespace cadabra;

Determinant::Determinant()
	{
	}

std::string Determinant::name() const
	{
	return "Determinant";
	}

std::string Determinant::unnamed_argument() const
	{
	return "object";
	}

bool Determinant::parse(Kernel&, keyval_t& keyvals) 
	{
	keyval_t::const_iterator kv=keyvals.find("object");
	if(kv!=keyvals.end())
		obj = kv->second;
	return true;
	}

void Determinant::validate(const Kernel&, const Ex& ) const
	{
	}

void Determinant::latex(std::ostream& str) const
	{
	str << name();
	}
