
#include "algorithms/order.hh"

order::order(Kernel& k, exptree& tr, iterator it, bool ac)
	: algorithm(k, tr), anticomm(ac)
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
	if(locate_(tr.begin(st), tr.end(st), locs)) {
		if(!(::is_sorted(locs.begin(), locs.end()))) {
			res=result_t::l_applied;

			std::vector<unsigned int> ordered(locs);
			std::sort(ordered.begin(), ordered.end());
			if(anticomm) {
				int osign=combin::ordersign(ordered.begin(), ordered.end(), locs.begin(), locs.end());
				if(osign!=1) {
					multiply(st->multiplier, osign);
					}
				}
			
			iterator orig_st=tr.begin(args_begin());
			for(unsigned int i=0; i<ordered.size(); ++i) {
				iterator dest_st=tr.begin(st);
				for(unsigned int k=0; k<ordered[i]; ++k)
					++dest_st;
//				txtout << "replacing " << *dest_st->name << " with " << *orig_st->name << std::endl;
				if((*orig_st->name).size()==0)
					tr.replace(dest_st, tr.begin(orig_st));
				else
					tr.replace(dest_st, orig_st);
				expression_modified=true;
				orig_st.skip_children();
				++orig_st;
				}
			}
		}
	prod_unwrap_single_term(st);

	return res;
	}

