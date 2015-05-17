
#include "properties/FilledTableau.hh"

std::string FilledTableau::name() const
	{
	return "FilledTableau";
	}

bool FilledTableau::parse(exptree& tr, exptree::iterator pat, exptree::iterator prop, keyval_t& keyvals)
	{
	keyval_t::const_iterator kv=keyvals.find("dimension");
	if(kv!=keyvals.end()) dimension=to_long(*(kv->second->multiplier));
	else dimension=-1;
	return true;
	}
