
#include "properties/Derivative.hh"
#include "Props.hh"
#include "Kernel.hh"

using namespace cadabra;

bool Derivative::parse(Kernel&, std::shared_ptr<Ex>, keyval_t& keyvals)
	{
	keyval_t::const_iterator ki=keyvals.begin();
	while(ki!=keyvals.end()) {
		if(ki->first=="to") {
			with_respect_to=ki->second;
			}
		++ki;
		}
	return true;
	}
			
multiplier_t Derivative::value(const Kernel& kernel, Ex::iterator it, const std::string& forcedlabel) const
	{
	const Properties& properties=kernel.properties;
	//	txtout << "!?!?" << std::endl;
	multiplier_t ret=0;

	Ex::sibling_iterator sib=it.begin();
	while(sib!=it.end()) {
		const WeightBase *gnb=properties.get_composite<WeightBase>(sib, forcedlabel);
		if(gnb) {
			multiplier_t tmp=gnb->value(kernel, sib, forcedlabel);
			if(sib->is_index()) ret-=tmp;
			else                ret+=tmp;
			//			txtout << *sib->name << " = " << tmp << std::endl;
			}
		++sib;
		}
	return ret;
	}

std::string Derivative::name() const
	{
	return "Derivative";
	}

unsigned int Derivative::size(const Properties& properties, Ex& tr, Ex::iterator it) const
	{
	it=properties.head<Derivative>(it);
	return TableauInherit::size(properties, tr, it);
	}

TableauBase::tab_t Derivative::get_tab(const Properties& properties, Ex& tr, Ex::iterator it, unsigned int num) const
	{
	it=properties.head<Derivative>(it);
	return TableauInherit::get_tab(properties, tr, it, num);
	}
