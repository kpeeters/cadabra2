
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

void RiemannTensor::validate(const Properties&, const exptree& pat) const
	{
//	if(pat.number_of_indices(pat.begin())!=4) 
//		throw std::consistency_error("RiemannTensor: need exactly 4 indices.");
	}

