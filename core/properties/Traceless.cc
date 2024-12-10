
#include "Traceless.hh"
#include "Exceptions.hh"

using namespace cadabra;

std::string Traceless::name() const
	{
	return "Traceless";
	}

bool Traceless::parse(Kernel&, keyval_t& keyvals)
	{
	keyval_t::const_iterator kv=keyvals.begin();
	while(kv!=keyvals.end()) {
		if(kv->first=="indices") {
			if(kv->second.is_string())
				index_set_names.insert(*kv->second.begin()->name);
			else
				throw ConsistencyException("Traceless: 'indices' argument should be a string.");
			}
		++kv;
		}
	return true;
	}
