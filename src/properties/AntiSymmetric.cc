
#include "properties/AntiSymmetric.hh"

std::string AntiSymmetric::name() const
	{
	return "AntiSymmetric";
	}

bool AntiSymmetric::parse(exptree& tr, exptree::iterator st, exptree::iterator it, keyval_t& kv)
	{
	return property::parse(tr,st,it,kv);
	}

unsigned int AntiSymmetric::size(exptree&, exptree::iterator) const
	{
	return 1;
	}

TableauBase::tab_t AntiSymmetric::get_tab(exptree& tr, exptree::iterator it, unsigned int num) const
	{
	assert(num==0);

	const AntiSymmetric *pd;
	for(;;) {
		pd=properties::get<AntiSymmetric>(it);
		if(!pd)
			it=tr.begin(it);
		else break;
		} 

	tab_t tab;
	for(unsigned int i=0; i<tr.number_of_children(it); ++i)
		tab.add_box(i,i);
	return tab;
	}

