
#include "Exceptions.hh"
#include "properties/Metric.hh"

Metric::Metric()
	{
	tab_t tab;
	tab.add_box(0,0);
	tab.add_box(0,1);
	tabs.push_back(tab);
	}

std::string Metric::name() const
	{
	return "Metric";
	}

bool Metric::parse(const Properties&, keyval_t& keyvals) 
	{
	keyval_t::const_iterator kv=keyvals.find("signature");
	signature=1;
	if(kv!=keyvals.end())
		signature=to_long(*(kv->second->multiplier));
	return true;
	}

void Metric::validate(const Properties& props, const Ex& tr) const
	{
	if(tr.number_of_children(tr.begin())!=2) 
		throw ArgumentException("Metric: needs exactly 2 indices.");
	}
