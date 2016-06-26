
#include "WeightInherit.hh"
#include "Kernel.hh"

std::string WeightInherit::name() const 
	{
	return "WeightInherit";
	}

WeightInherit::WeightException::WeightException(const std::string& str)
	: ConsistencyException(str)
	{
	}

bool WeightInherit::parse(const Kernel& k, keyval_t& kv)
	{
	keyval_t::const_iterator tpit=kv.find("type");
	if(tpit!=kv.end()) {
		if(*tpit->second->name=="multiplicative") combination_type=multiplicative;
		else                                      combination_type=additive;
		}
	else combination_type=multiplicative;

	tpit = kv.find("self");
	if(tpit!=kv.end()) {
		value_self=*tpit->second->multiplier;
		}
	else value_self=0;

	auto ret = WeightBase::parse(k, kv);
	return ret;
	}

multiplier_t WeightInherit::value(const Kernel& kernel, Ex::iterator it, const std::string& forcedlabel) const
	{
	const Properties& properties=kernel.properties;

	multiplier_t ret=0;
	bool first_term=true;

	// std::cerr << "calling inherit on " << *it->name << " " << &(*it) << " " << forcedlabel << std::endl;
	Ex::sibling_iterator sib=it.begin();
	while(sib!=it.end()) {
		 if(!sib->is_index()) {
			  if(combination_type==multiplicative) {
					const WeightBase *gnb=properties.get_composite<WeightBase>(sib, forcedlabel);
					// std::cerr << "finding weight for " << Ex(sib) << std::endl;
					if(gnb) {
						multiplier_t tmp=gnb->value(kernel, sib, forcedlabel);
						ret+=tmp;
						}
				  }
			  else {
				  multiplier_t thisone=0;
				  const WeightBase *gnb=properties.get_composite<WeightBase>(sib, forcedlabel);
				  if(gnb) thisone=gnb->value(kernel, sib, forcedlabel);
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

