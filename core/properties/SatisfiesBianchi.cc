
#include "properties/SatisfiesBianchi.hh"
#include "IndexIterator.hh"

using namespace cadabra;

std::string SatisfiesBianchi::name() const
	{
	return "SatisfiesBianchi";
	}

unsigned int SatisfiesBianchi::size(const Properties& properties, Ex& tr, Ex::iterator it) const
	{
	Ex::sibling_iterator chld=tr.begin(it);
	bool indexfirst=false;
	if(chld->fl.parent_rel!=str_node::p_none) {
		indexfirst=true;
		++chld;
		}
	assert(chld->fl.parent_rel==str_node::p_none);
	const TableauBase *tb=properties.get<TableauBase>(chld);

	if(!tb) return 0;

	assert(tb->size(properties, tr, chld)==1); // Does this make sense otherwise?

	return 1;
	}

TableauBase::tab_t SatisfiesBianchi::get_tab(const Properties& properties, Ex& tr, Ex::iterator it, unsigned int) const
	{
	// Take the tableau of the child, increase all indices by
	// one if the derivative index sits on the first position,
	// and then add a box on the first row corresponding to the
	// derivative.

	Ex::sibling_iterator chld=tr.begin(it);
	bool indexfirst=false;
	if(chld->fl.parent_rel!=str_node::p_none) {
		indexfirst=true;
		++chld;
		}
	assert(chld->fl.parent_rel==str_node::p_none);
	//	txtout << *chld->name << std::endl;
	const TableauBase *tb=properties.get<TableauBase>(chld);
	assert(tb);
	//	txtout << "got child TableauBase" << std::endl;

	assert(tb->size(properties, tr, chld)==1);
	tab_t thetab=tb->get_tab(properties, tr, chld, 0);
	//	txtout << "got child tab" << std::endl;
	if(indexfirst) {
		for(unsigned int r=0; r<thetab.number_of_rows(); ++r)
			for(unsigned int c=0; c<thetab.row_size(r); ++c)
				thetab(r,c)+=1;
		thetab.add_box(0, 0);
		} else {
		index_iterator ii=index_iterator::begin(properties, it);
		unsigned int pos=0;
		while(ii!=index_iterator::end(properties, it)) {
			++ii;
			++pos;
			}
		thetab.add_box(0, pos-1);
		}

	return thetab;
	}


