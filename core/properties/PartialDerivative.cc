
#include "Algorithm.hh"
#include "properties/PartialDerivative.hh"

using namespace cadabra;

unsigned int PartialDerivative::size(const Properties& properties, Ex& tr, Ex::iterator it) const
	{
	return Derivative::size(properties, tr, it)+1;
	}


TableauBase::tab_t PartialDerivative::get_tab(const Properties& properties, Ex& tr, Ex::iterator it, unsigned int num) const
	{
	it=properties.head<PartialDerivative>(it);

	bool indices_first=tr.begin(it)->is_index();
	Ex::sibling_iterator argnode=tr.begin(it);
	unsigned int number_of_indices=0;
	while(argnode->is_index()) { ++argnode; ++number_of_indices; }
	unsigned int arg_indices=tr.number_of_children(argnode);

	if(num==0) { // symmetry of the derivative indices
		tab_t tab;
		int i=0;
		index_iterator indit=index_iterator::begin(properties, it);
		if(!indices_first) {
			for(unsigned int k=0; k<arg_indices; ++k) ++indit;
			i+=arg_indices;
			}
		while(indit!=index_iterator::end(properties, it)) {
			if(tr.parent((Ex::iterator)indit)!=it) break;
//			txtout << "T: " << i << " " << *indit->name << std::endl;
			tab.add_box(0, i);
			++i;
			++indit;
			}
		return tab;
		}
	else {
		return Derivative::get_tab(properties, tr, it, num-1);
		}
	}

std::string PartialDerivative::name() const
	{
	return "PartialDerivative";
	}

