
#include "IndexIterator.hh"
#include "Exceptions.hh"
#include "properties/WeylTensor.hh"

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

void WeylTensor::validate(const Properties& props, const exptree& pat) const
	{
	if(number_of_indices(props, pat.begin())!=4) 
		throw ConsistencyException("WeylTensor: need exactly 4 indices.");
	}

void WeylTensor::display(std::ostream& str) const
	{
	TableauSymmetry::display(str);
	Traceless::display(str);
	}

