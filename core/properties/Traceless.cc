
#include "Traceless.hh"

using namespace cadabra;

std::string Traceless::name() const
	{
	return "Traceless";
	}

bool Traceless::parse(Kernel&, keyval_t& keyvals)
	{
	keyval_t::const_iterator kv=keyvals.begin();
	while(kv!=keyvals.end()) {
		if(kv->first=="indices") index_set_names.insert(*kv->second->name);
		++kv;
		}
	return true;
	}
