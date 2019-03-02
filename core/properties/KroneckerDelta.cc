
#include "properties/KroneckerDelta.hh"
#include "Exceptions.hh"

using namespace cadabra;

std::string KroneckerDelta::name() const
	{
	return "KroneckerDelta";
	}

unsigned int KroneckerDelta::size(const Properties&, Ex&, Ex::iterator) const
	{
	return 1;
	}

TableauBase::tab_t KroneckerDelta::get_tab(const Properties& properties, Ex& tr, Ex::iterator it, unsigned int num) const
	{
	assert(num==0);

	const KroneckerDelta *pd;
	for(;;) {
		pd=properties.get<KroneckerDelta>(it);
		if(!pd)
			it=tr.begin(it);
		else break;
		}

	if(tr.number_of_children(it)%2!=0)
		throw ConsistencyException("Encountered a KroneckerDelta object with an odd number of indices.");

	bool onlytwo=false;
	if(tr.number_of_children(it)==2)
		onlytwo=true;

	tab_t tab;
	for(unsigned int i=0; i<tr.number_of_children(it); i+=2) {
		tab.add_box(i/2,i);
		//		if(onlytwo)
		tab.add_box(i/2,i+1);
		}
	return tab;
	}
