
#include "algorithms/reduce_sub.hh"

reduce_sub::reduce_sub(Kernel& k, exptree& tr)
	: Algorithm(k, tr)
	{
	}

bool reduce_sub::can_apply(iterator st)
	{
	if(*st->name!="\\sub") return false;
	return true;
	}

Algorithm::result_t reduce_sub::apply(iterator& it)
	{
	assert(tr.number_of_children(it)>1); // To guarantee that we have really cleaned up that old stuff.

	it->name=name_set.insert("\\sum").first;
	exptree::sibling_iterator sit=tr.begin(it);

	// Make sure that all terms have the right sign, and zeroes are removed.
	if(*sit->multiplier==0) sit=tr.erase(sit);
	else                    ++sit;

	while(sit!=tr.end(it)) {
		if(*sit->multiplier==0)
			sit=tr.erase(sit);
		else {
			flip_sign(sit->multiplier);
			++sit;
			}
		}

	// Single-term situation: remove the \sum.
	assert(tr.number_of_children(it)>0);
	if(tr.number_of_children(it)==1) {
		sit=tr.begin(it);
		sit->fl.parent_rel=it->fl.parent_rel;
		sit->fl.bracket=it->fl.bracket;
		tr.flatten(it);
		it=tr.erase(it);
		}

	expression_modified=true;
	return l_applied;
	}

