
#include "WeightInherit.hh"

std::string WeightInherit::name() const 
	{
	return "WeightInherit";
	}

WeightInherit::WeightException::WeightException(const std::string& str)
	: ConsistencyException(str)
	{
	}

bool WeightInherit::parse(exptree& tr, exptree::iterator pat, exptree::iterator prop, keyval_t& kv)
	{
	keyval_t::const_iterator tpit=kv.find("type");
	if(tpit!=kv.end()) {
		if(*tpit->second->name=="Multiplicative") combination_type=multiplicative;
		else                                      combination_type=additive;
		}
	else combination_type=multiplicative;

	tpit = kv.find("self");
	if(tpit!=kv.end()) {
		value_self=*tpit->second->multiplier;
		}
	else value_self=0;

	return true;
	}

multiplier_t WeightInherit::value(const Properties& properties, exptree::iterator it, const std::string& forcedlabel) const
	{
	multiplier_t ret=0;
	bool first_term=true;

//	txtout << "calling inherit on " << *it->name << " " << &(*it) << " " << forcedlabel << std::endl;
	exptree::sibling_iterator sib=it.begin();
	while(sib!=it.end()) {
		 if(!sib->is_index()) {
			  if(combination_type==multiplicative) {
					const WeightBase *gnb=properties.get_composite<WeightBase>(sib, forcedlabel);
					if(gnb) {
						multiplier_t tmp=gnb->value(properties, sib, forcedlabel);
						 ret+=tmp;
						 }
					}
			  else {
					multiplier_t thisone=0;
					const WeightBase *gnb=properties.get_composite<WeightBase>(sib, forcedlabel);
					if(gnb) thisone=gnb->value(properties, sib, forcedlabel);
					else    thisone=0;
					if(first_term) {
						first_term=false;
						ret=thisone;
						}
					else if(ret!=thisone) { // the weights in the sum are not uniform
						throw WeightException("Encountered sum with un-equal weight terms.");
						}
				  }
			 }
		 ++sib;
		}

	ret+=value_self;  // Add our own weight.

	return ret;
	}

