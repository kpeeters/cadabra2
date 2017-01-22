

#include "Exceptions.hh"
#include "properties/InverseMetric.hh"

using namespace cadabra;

InverseMetric::InverseMetric()
	{
	tab_t tab;
	tab.add_box(0,0);
	tab.add_box(0,1);
	tabs.push_back(tab);
	}

std::string InverseMetric::name() const
	{
	return "InverseMetric";
	}

bool InverseMetric::parse(const Kernel&, keyval_t& keyvals) 
	{
	keyval_t::const_iterator kv=keyvals.find("signature");
	signature=1;
	if(kv!=keyvals.end())
		signature=to_long(*(kv->second->multiplier));
	return true;
	}

void InverseMetric::validate(const Kernel&, const Ex& tr) const
	{
	if(tr.number_of_children(tr.begin())!=2) 
		throw ArgumentException("InverseMetric: needs exactly 2 indices.");
	}
