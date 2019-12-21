
#include "Algorithm.hh"
#include "Exceptions.hh"
#include "IndexIterator.hh"
#include "Kernel.hh"
#include "properties/EpsilonTensor.hh"
#include "properties/Integer.hh"

using namespace cadabra;

std::string EpsilonTensor::name() const
	{
	return "EpsilonTensor";
	}

bool EpsilonTensor::parse(Kernel&, keyval_t& keyvals)
	{
	keyval_t::const_iterator kv=keyvals.find("metric");
	if(kv!=keyvals.end()) metric=Ex(kv->second);

	kv=keyvals.find("delta");
	if(kv!=keyvals.end()) krdelta=Ex(kv->second);

	return true;
	}

void EpsilonTensor::validate(const Kernel& kernel, const Ex& pat) const
	{
	index_iterator indit=index_iterator::begin(kernel.properties, pat.begin());
	auto ind1=kernel.properties.get<Indices>(indit, true);
	auto ind2=kernel.properties.get<Integer>(indit, true);
	if(ind2) {
		// This might have to change when index parents become well supported
		unsigned int dim=to_long(*ind2->difference.begin()->multiplier);
		if(Algorithm::number_of_indices(kernel.properties, pat.begin())!=dim)
			throw ConsistencyException("Number of indices does not match the number of values they can take.");
		}
	EpsilonTensor *ptr = const_cast<EpsilonTensor*>(this);
	ptr->index_set_name=ind1->set_name;
	}
