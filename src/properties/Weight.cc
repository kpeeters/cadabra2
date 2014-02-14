
#include "Weight.hh"

std::string Weight::name() const 
	{
	return "Weight";
	}

bool Weight::parse(exptree& tr, exptree::iterator pat, exptree::iterator prop, keyval_t& kv)
	{
	keyval_t::const_iterator kvit=kv.find("value");
	if(kvit!=kv.end()) value_=*kvit->second->multiplier;
	else               value_=1;

	return true;
	}

multiplier_t Weight::value(exptree::iterator, const std::string& forcedlabel) const
	{
	if(forcedlabel!=label) return -1;
	return value_;
	}
