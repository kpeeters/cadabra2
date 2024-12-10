
#include "WeightInherit.hh"
#include "Kernel.hh"
#include "Exceptions.hh"

using namespace cadabra;

WeightInherit::~WeightInherit()
	{
	}

std::string WeightInherit::name() const
	{
	return "WeightInherit";
	}

WeightInherit::WeightException::WeightException(const std::string& str)
	: ConsistencyException(str)
	{
	}

bool WeightInherit::parse(Kernel& k, std::shared_ptr<Ex> ex, keyval_t& kv)
	{
	keyval_t::const_iterator tpit=kv.find("type");
	if(tpit!=kv.end()) {
		if(tpit->second.equals("multiplicative")) combination_type=multiplicative;
		else if(tpit->second.equals("additive"))  combination_type=additive;
		else if(tpit->second.equals("power"))     combination_type=power;
		else throw ArgumentException("weight type must be 'multiplicative', 'additive' or 'power'.");
		}
	else combination_type=multiplicative;

	tpit = kv.find("self");
	if(tpit!=kv.end()) {
		if(tpit->second.is_rational()==false)
			throw ConsistencyException("WeightInherit: 'self' value should be an explicit rational.");
		
		value_self=*tpit->second.begin()->multiplier;
		}
	else value_self=0;

	auto ret = WeightBase::parse(k, ex, kv);
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
			switch(combination_type) {
				case multiplicative: {
					const WeightBase *gnb=properties.get<WeightBase>(sib, forcedlabel);
					// std::cerr << "finding weight for " << Ex(sib) << std::endl;
					if(gnb) {
						multiplier_t tmp=gnb->value(kernel, sib, forcedlabel);
						ret+=tmp;
						}
					}
				break;
				case additive: {
					multiplier_t thisone=0;
					const WeightBase *gnb=properties.get<WeightBase>(sib, forcedlabel);
					if(gnb) thisone=gnb->value(kernel, sib, forcedlabel);
					else    thisone=0;
					if(first_term) {
						first_term=false;
						ret=thisone;
						}
					else if(ret!=thisone) {   // the weights in the sum are not uniform
						throw WeightException("Encountered sum with un-equal weight terms.");
						}
					}
				break;
				case power: {
					const WeightBase *gnb=properties.get<WeightBase>(sib, forcedlabel);
					if(gnb) {
						multiplier_t tmp=gnb->value(kernel, sib, forcedlabel);
						++sib;
						if(sib==it.end() || sib->is_rational()==false)
							throw RuntimeException("Can only handle numerical exponents for weight counting.");

						ret+=(*sib->multiplier)*tmp;
						sib=it.end();
						continue;
						}
					}
				break;
				}
			}
		++sib;
		}

	ret+=value_self;  // Add our own weight.

	return ret;
	}

