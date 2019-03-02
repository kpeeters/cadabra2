
#include "algorithms/flatten_sum.hh"

using namespace cadabra;

flatten_sum::flatten_sum(const Kernel& k, Ex& tr)
	: Algorithm(k, tr), make_consistent_only(false)
	{
	}

bool flatten_sum::can_apply(iterator it)
	{
	if(*it->name!="\\sum") return false;
	if(tr.number_of_children(it)==1 || tr.number_of_children(it)==0) return true;
	sibling_iterator facs=tr.begin(it);
	while(facs!=tr.end(it)) {
		if(*(*facs).name=="\\sum")
			return true;
		++facs;
		}
	return false;
	}

Algorithm::result_t flatten_sum::apply(iterator &it)
	{
	result_t ret=result_t::l_no_action;

	assert(*it->name=="\\sum");

	long num=tr.number_of_children(it);
	if(num==1 && ! tr.begin(it)->is_range_wildcard() ) {
		multiply(tr.begin(it)->multiplier, *it->multiplier);
		tr.flatten(it);
		it=tr.erase(it);
		ret=result_t::l_applied;
		} else if(num==0) {
		node_zero(it);
		ret=result_t::l_applied;
		} else {
		sibling_iterator facs=tr.begin(it);
		str_node::bracket_t btype_par=facs->fl.bracket;
		while(facs!=tr.end(it)) {
			if(facs->fl.bracket!=str_node::b_none)
				btype_par=facs->fl.bracket;
			++facs;
			}
		facs=tr.begin(it);
		while(facs!=tr.end(it)) {
			if(*facs->name=="\\sum") {
				sibling_iterator terms=tr.begin(facs);
				str_node::bracket_t btype=terms->fl.bracket;
				if(!make_consistent_only || btype==str_node::b_none || btype==str_node::b_no) {
					ret=result_t::l_applied;
					sibling_iterator tmp=facs;
					++tmp;
					while(terms!=tr.end(facs)) {
						multiply(terms->multiplier,*facs->multiplier);
						terms->fl.bracket=btype_par;
						//						if(terms->fl.bracket==str_node::b_none)
						//							terms->fl.bracket=facs->fl.bracket;
						++terms;
						}
					tr.flatten(facs);
					tr.erase(facs);
					facs=tmp;
					} else ++facs;
				} else ++facs;
			}
		}
	return ret;
	}
