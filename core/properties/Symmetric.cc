
#include "Algorithm.hh"
#include "properties/Symmetric.hh"

using namespace cadabra;

std::string Symmetric::name() const
	{
	return "Symmetric";
	}

unsigned int Symmetric::size(const Properties&, Ex&, Ex::iterator) const
	{
	return 1;
	}

TableauBase::tab_t Symmetric::get_tab(const Properties& pr, Ex& tr, Ex::iterator it, unsigned int num) const
	{
	assert(num==0);

	const Symmetric *pd;
	for(;;) {
		pd=pr.get<Symmetric>(it);
		if(!pd)
			it=tr.begin(it);
		else break;
		}

	tab_t tab;
	for(unsigned int i=0; i<Algorithm::number_of_indices(pr, it); ++i)
		tab.add_box(0,i);
	return tab;
	}

