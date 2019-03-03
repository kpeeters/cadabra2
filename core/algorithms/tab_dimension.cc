
#include "Cleanup.hh"
#include "algorithms/tab_dimension.hh"

using namespace cadabra;

tabdimension::tabdimension(const Kernel& k, Ex& ex)
	: Algorithm(k, ex)
	{
	}

bool tabdimension::can_apply(iterator it)
	{
	dimension=-1;

	tab=kernel.properties.get<Tableau>(it);
	if(tab) {
		dimension=tab->dimension;
		if(dimension>0)
			return true;
		}

	ftab=kernel.properties.get<FilledTableau>(it);
	if(ftab) {
		dimension=ftab->dimension;
		// std::cerr << "dimension " << dimension << std::endl;
		if(dimension>0)
			return true;
		}

	return false;
	}

Algorithm::result_t tabdimension::apply(iterator& it)
	{
	// std::cerr << "applying at " << it << std::endl;
	if(ftab) {
		yngtab::filled_tableau<Ex> one;

		sibling_iterator sib=tr.begin(it);
		unsigned int currow=0;
		while(sib!=tr.end(it)) {
			if(*sib->name=="\\comma") {
				sibling_iterator sib2=tr.begin(sib);
				while(sib2!=tr.end(sib)) {
					one.add_box(currow, Ex(sib2));
					++sib2;
					}
				}
			else one.add_box(currow, Ex(sib));
			++sib;
			++currow;
			}
		node_one(it);
		multiply(it->multiplier, one.dimension(dimension));
		}
	else {
		yngtab::tableau one;
		sibling_iterator sib=tr.begin(it);
		while(sib!=tr.end(it)) {
			one.add_row(to_long(*sib->multiplier));
			++sib;
			}
		node_one(it);
		multiply(it->multiplier, one.dimension(dimension));
		}
	cleanup_dispatch(kernel, tr, it);
	return result_t::l_applied;
	}

