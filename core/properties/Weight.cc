
#include "Weight.hh"

using namespace cadabra;

std::string Weight::name() const 
	{
	return "Weight";
	}

bool Weight::parse(Kernel& k, std::shared_ptr<Ex> ex, keyval_t& kv)
	{
	keyval_t::const_iterator kvit=kv.find("value");
	if(kvit!=kv.end()) value_=*kvit->second->multiplier;
	else               value_=1;

	return WeightBase::parse(k, ex, kv);
	}

multiplier_t Weight::value(const Kernel&, Ex::iterator, const std::string& forcedlabel) const
	{
	if(forcedlabel!=label) return -1;
	return value_;
	}
