
#include "algorithms/order.hh"
#include "Combinatorics.hh"

order::order(Kernel& k, Ex& tr, Ex& objs, bool ac)
	: Algorithm(k, tr), objects(objs), anticomm(ac)
	{
	}

bool order::can_apply(iterator st)
	{
	if(*(st->name)!="\\prod")
		return(is_single_term(st));
	return true;
	}

Algorithm::result_t order::apply(iterator& st)
	{
	result_t res=result_t::l_no_action;

	prod_wrap_single_term(st);

	std::vector<unsigned int> locs;
	if(locate_object_set(objects, tr.begin(st), tr.end(st), locs)) {

		if(!(std::is_sorted(locs.begin(), locs.end()))) {
			res=result_t::l_applied;

			std::vector<unsigned int> ordered(locs);
			std::sort(ordered.begin(), ordered.end());
			if(anticomm) {
				int osign=combin::ordersign(ordered.begin(), ordered.end(), locs.begin(), locs.end());
				if(osign!=1) {
					multiply(st->multiplier, osign);
					}
				}
			
			// \expression{\comma{A}{B}}
			sibling_iterator orig_st=objects.begin(objects.begin(objects.begin()));

			for(unsigned int i=0; i<ordered.size(); ++i) {
				iterator dest_st=tr.begin(st);
				for(unsigned int k=0; k<ordered[i]; ++k)
					++dest_st;
				if((*orig_st->name).size()==0)
					tr.replace(dest_st, tr.begin(orig_st));
				else
					tr.replace(dest_st, orig_st);

				++orig_st;
				}

			res=result_t::l_applied;
			}
		}
	prod_unwrap_single_term(st);

	return res;
	}

