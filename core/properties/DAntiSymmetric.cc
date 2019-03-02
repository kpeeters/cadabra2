
#include "properties/DAntiSymmetric.hh"

using namespace cadabra;

std::string DAntiSymmetric::name() const
	{
	return "DAntiSymmetric";
	}

unsigned int DAntiSymmetric::size(const Properties&, Ex&, Ex::iterator) const
	{
	return 1;
	}

TableauBase::tab_t DAntiSymmetric::get_tab(const Properties& properties, Ex& tr, Ex::iterator it, unsigned int num) const
	{
	assert(num==0);

	const DAntiSymmetric *pd;
	for(;;) {
		pd=properties.get<DAntiSymmetric>(it);
		if(!pd)
			it=tr.begin(it);
		else break;
		}

	tab_t tab;
	tab.add_box(0,1);
	tab.add_box(0,0); // these were in the wrong order!!!
	for(unsigned int i=2; i<tr.number_of_children(it); ++i)
		tab.add_box(i-1,i);
	return tab;
	}
