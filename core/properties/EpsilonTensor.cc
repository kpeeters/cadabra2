
#include "properties/EpsilonTensor.hh"

std::string EpsilonTensor::name() const
	{
	return "EpsilonTensor";
	}

bool EpsilonTensor::parse(const Kernel& k, keyval_t& keyvals)
	{
	keyval_t::const_iterator kv=keyvals.find("metric");
	if(kv!=keyvals.end()) metric=Ex(kv->second);

	kv=keyvals.find("delta");
	if(kv!=keyvals.end()) krdelta=Ex(kv->second);

	return true;
	}
