
#include "properties/FilledTableau.hh"

using namespace cadabra;

std::string FilledTableau::name() const
	{
	return "FilledTableau";
	}

bool FilledTableau::parse(Kernel&, keyval_t& keyvals)
	{
	keyval_t::const_iterator kv=keyvals.find("dimension");
	if(kv!=keyvals.end()) dimension=to_long(*(kv->second.begin()->multiplier));
	else dimension=-1;
	return true;
	}
