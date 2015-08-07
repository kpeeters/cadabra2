
#include "IndexIterator.hh"
#include "Exceptions.hh"
#include "properties/RiemannTensor.hh"

RiemannTensor::RiemannTensor()
	{
	tab_t tab;
	tab.add_box(0,0);
	tab.add_box(0,2);
	tab.add_box(1,1);
	tab.add_box(1,3);
	tabs.push_back(tab);
	}

std::string RiemannTensor::name() const
	{
	return "RiemannTensor";
	}

void RiemannTensor::validate(const Properties& props, const Ex& pat) const
	{
	if(number_of_indices(props, pat.begin())!=4) 
		throw ConsistencyException("RiemannTensor: need exactly 4 indices.");
	}

