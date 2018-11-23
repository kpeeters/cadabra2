
#include "Exceptions.hh"
#include "properties/Trace.hh"

using namespace cadabra;

Trace::Trace()
	{
	}

std::string Trace::name() const
	{
	return "Trace";
	}

std::string Trace::unnamed_argument() const
	{
	return "object";
	}

bool Trace::parse(Kernel&, keyval_t& keyvals) 
	{
	keyval_t::const_iterator kv=keyvals.find("object");
	if(kv!=keyvals.end())
		obj = kv->second;
	return true;
	}

void Trace::validate(const Kernel&, const Ex& tr) const
	{
	}

void Trace::latex(std::ostream& str) const
	{
	str << name();
	}
