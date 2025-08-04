
#include "IndexIterator.hh"
#include "Exceptions.hh"
#include "Kernel.hh"
#include "Algorithm.hh"
#include "properties/RiemannTensor.hh"

// #define DEBUG __FILE__
#include "Debug.hh"

using namespace cadabra;

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

void RiemannTensor::validate(Kernel& kernel, std::shared_ptr<Ex> pat) const
	{
	const Properties& props=kernel.properties;
	if(Algorithm::number_of_indices(props, pat->begin())!=4)
		throw ConsistencyException("RiemannTensor: need exactly 4 indices.");

	int upper=0, lower=0;

	index_iterator ii=index_iterator::begin(kernel.properties, pat->begin());
	while(ii != index_iterator::end(kernel.properties, pat->begin()) ) {
		if(ii->fl.parent_rel==str_node::p_sub)        ++lower;
		else if(ii->fl.parent_rel==str_node::p_super) ++upper;
		else throw ConsistencyException("RiemannTensor: all indices need to be either upper or lower.");
		++ii;
		}
	DEBUGLN( std::cerr << "RiemannTensor:validate: found " << upper << "/" << lower << " upper/lower indices." << std::endl; );
	if(upper!=4 && lower!=4)
		throw ConsistencyException("RiemannTensor: all indices need to be either upper or lower.");

	// Also inject property with indices all moved to the other position.
	std::shared_ptr<Ex> ex_other = std::make_shared<Ex>(pat->begin());
	Ex::sibling_iterator sib=ex_other->begin(ex_other->begin());
	while(sib!=ex_other->end(ex_other->begin())) {
		sib->fl.parent_rel = (upper==4 ? str_node::p_sub : str_node::p_super);
		++sib;
		}
	kernel.properties.master_insert(*ex_other, new RiemannTensor());
	}




