
#include "algorithms/eliminate_metric.hh"

eliminate_metric::eliminate_metric(const Kernel& k, Ex& e)
	: eliminate_converter(k, e)
	{
	}

bool eliminate_metric::is_conversion_object(iterator fit) const 
	{
	const Metric        *vb=properties::get<Metric>(fit);
	const InverseMetric *ivb=properties::get<InverseMetric>(fit);
	
	if(vb || ivb)  return true;
	else return false;
	}

eliminate_converter::eliminate_converter(const Kernel& k, Ex& e)
	: Algorithm(k, e)
	{
	}

bool eliminate_converter::can_apply(iterator it)
	{
	if(*it->name=="\\prod") return true;
	return false;
	}

bool eliminate_converter::handle_one_index(iterator ind1, iterator ind2, iterator fit, sibling_iterator objs)
	{
	bool replaced=false;

	std::pair<index_map_t::const_iterator,index_map_t::const_iterator> locs=
		ind_dummy.equal_range(Ex(ind1));
	if(std::distance(locs.first, locs.second)==2) {
//				txtout << "index " << *ind1->name << std::endl;
		while(locs.first!=locs.second) {
			if(locs.first->second!=(iterator)ind1) {
				// Does this index sit on an object in the "preferred form" list?
				// (if there is no preferred form, always eliminate)
				if(separated_by_derivative(locs.first->second, ind2, fit)==false) {
					if(objs==args_end()) { // no
						tr.move_ontop(locs.first->second, iterator(ind2))->fl.parent_rel=ind2->fl.parent_rel;;
						fit=tr.erase(fit);
						expression_modified=true;
						replaced=true;
						}
					else { // yes
						iterator par=tr.parent(locs.first->second);
						sibling_iterator prefit=tr.begin(objs);
						while(prefit!=tr.end(objs)) {
							if(subtree_equal(prefit, par, -1, false)) {
								tr.move_ontop(locs.first->second, iterator(ind2))->fl.parent_rel=ind2->fl.parent_rel;;
								fit=tr.erase(fit);
								expression_modified=true;
								replaced=true;
								break;
								}
							++prefit;
							}
						}
					}
				}
			if(replaced) 
				return true;
			++locs.first;
			}
		}
	return false;
	}


Algorithm::result_t eliminate_converter::apply(iterator& it)
	{
	result_t res=result_t::l_no_action;


	// Put arguments in canonical form.

	sibling_iterator objs=args_begin();

	if(objs!=args_end())
		if(*objs->name!="\\comma")
			objs=tr.wrap(objs, str_node("\\comma"));

	ind_free.clear();
	ind_dummy.clear();

	classify_indices(it, ind_free, ind_dummy);

	// Run over all factors, find metrics, figure out whether they can
	// be used to turn the indices on the other tensors to preferred type.

	sibling_iterator fit=tr.begin(it);
	while(fit!=tr.end(it)) {
		if(is_conversion_object(fit)) {
			sibling_iterator ind1=tr.begin(fit), ind2=ind1;
			++ind2;

         // 1st index to 2nd index conversion?
			if(handle_one_index(ind1, ind2, fit, objs))
				break;
			
			// 2nd index to 1st index conversion?
			if(handle_one_index(ind2, ind1, fit, objs))
				break;
			}
		++fit;
		}

	if(res==result_t::l_applied)
		cleanup_dispatch(tr, k, it);

	return res;
	}
