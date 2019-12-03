
#include "Traceless.hh"

using namespace cadabra;

std::string Traceless::name() const
	{
	return "Traceless";
	}

bool Traceless::parse(Kernel&, keyval_t& keyvals)
	{
	keyval_t::const_iterator kv=keyvals.find("indices");
	if(kv!=keyvals.end())
		index_set_name=*(kv->second->name);
	return true;
	}
