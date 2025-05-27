
#include "Exceptions.hh"
#include "IndexIterator.hh"
#include "Kernel.hh"
#include "properties/Metric.hh"
#include "properties/Indices.hh"

using namespace cadabra;

Metric::Metric()
	{
	//	tab_t tab;
	//	tab.add_box(0,0);
	//	tab.add_box(0,1);
	//	tabs.push_back(tab);
	}

std::string Metric::name() const
	{
	return "Metric";
	}

bool Metric::parse(Kernel&, keyval_t& keyvals)
	{
	keyval_t::const_iterator kv=keyvals.find("signature");
	signature=1;
	if(kv!=keyvals.end())
		signature=to_long(*(kv->second.begin()->multiplier));
	return true;
	}

void Metric::validate(const Kernel& kernel, const Ex& tr) const
	{
	auto st = index_iterator::begin(kernel.properties, tr.begin());
	auto nd = index_iterator::end(kernel.properties, tr.begin());
	size_t num=0;
	while(st != nd) {
		const Indices *ind = kernel.properties.get<Indices>(st);
		if(ind==nullptr)
			throw ArgumentException("Metric: indices of a metric object must have been declared as Indices.");

		if(ind->position_type==Indices::position_t::free)
			throw ArgumentException("Metric: indices of a metric object must have position 'fixed' or 'independent'.");

		++num;
		++st;
		}
	if(num!=2) {
		throw ArgumentException("Metric: needs exactly 2 indices.");
		}
	}

void Metric::latex(std::ostream& str) const
	{
	str << name();
	}
