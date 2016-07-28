
#include "Cleanup.hh"
#include "algorithms/eliminate_metric.hh"
#include "properties/Metric.hh"
#include "properties/InverseMetric.hh"

eliminate_metric::eliminate_metric(const Kernel& k, Ex& e, Ex& pref)
	: eliminate_converter(k, e, pref)
	{
	}

bool eliminate_metric::is_conversion_object(iterator fit) const 
	{
	const Metric        *vb=kernel.properties.get<Metric>(fit);
	const InverseMetric *ivb=kernel.properties.get<InverseMetric>(fit);
	
	if(vb || ivb)  return true;
	else return false;
	}

eliminate_converter::eliminate_converter(const Kernel& k, Ex& e, Ex& pref)
	: Algorithm(k, e), preferred(pref)
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

	// For a conversion to be possible, we need one upper and one lower index
	// (for position!=free) or two indices (for position=free).

	auto locs= ind_dummy.equal_range(Ex(ind1));
	int num1=std::distance(locs.first, locs.second);
	Ex other(ind1);
	other.begin()->flip_parent_rel();
	locs = ind_dummy.equal_range(other);
	int num2=std::distance(locs.first, locs.second);

	if(num1==1 && num2==1) {
		while(locs.first!=locs.second) {
			if(locs.first->second!=(iterator)ind1) {
				// Does this index sit on an object in the "preferred form" list?
				// (if there is no preferred form, always eliminate)
				if(separated_by_derivative(locs.first->second, ind2, fit)==false) {
					if(objs==preferred.end()) { // no
						tr.move_ontop(locs.first->second, iterator(ind2))->fl.parent_rel=ind2->fl.parent_rel;;
						fit=tr.erase(fit);
						replaced=true;
						}
					else { // yes
						iterator par=tr.parent(locs.first->second);
						sibling_iterator prefit=tr.begin(objs);
						while(prefit!=tr.end(objs)) {
							if(subtree_equal(&kernel.properties, prefit, par, -1, false)) {
								tr.move_ontop(locs.first->second, iterator(ind2))->fl.parent_rel=ind2->fl.parent_rel;;
								fit=tr.erase(fit);
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

	sibling_iterator objs=preferred.begin();

	if(objs!=preferred.end())
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
			if(handle_one_index(ind1, ind2, fit, objs)) {
				res=result_t::l_applied;
				break;
				}
			
			// 2nd index to 1st index conversion?
			if(handle_one_index(ind2, ind1, fit, objs)) {
				res=result_t::l_applied;
				break;
				}
			}
		++fit;
		}

	if(res==result_t::l_applied)
		cleanup_dispatch(kernel, tr, it);

	return res;
	}
