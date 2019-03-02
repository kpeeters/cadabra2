
#include "IndexIterator.hh"
#include "Exceptions.hh"
#include "Kernel.hh"
#include "Algorithm.hh"
#include "properties/WeylTensor.hh"

using namespace cadabra;

WeylTensor::WeylTensor()
	{
	tab_t tab;
	tab.add_box(0,0);
	tab.add_box(0,2);
	tab.add_box(1,1);
	tab.add_box(1,3);
	tabs.push_back(tab);
	}

std::string WeylTensor::name() const
	{
	return "WeylTensor";
	}

void WeylTensor::validate(const Kernel& kernel, const Ex& pat) const
	{
	if(Algorithm::number_of_indices(kernel.properties, pat.begin())!=4)
		throw ConsistencyException("WeylTensor: need exactly 4 indices.");
	}

void WeylTensor::latex(std::ostream& str) const
	{
	TableauSymmetry::latex(str);
	Traceless::latex(str);
	}

