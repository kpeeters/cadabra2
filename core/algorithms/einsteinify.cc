
#include "algorithms/einsteinify.hh"

using namespace cadabra;

einsteinify::einsteinify(const Kernel& k, Ex& e, Ex& m)
	: Algorithm(k, e), metric(m)
	{
	}

bool einsteinify::can_apply(iterator it)
	{
	if(*it->name=="\\prod") return true;
	return false;
	}

Algorithm::result_t einsteinify::apply(iterator& it)
	{
	result_t res=result_t::l_no_action;

	bool insert_metric=false;
	if(*metric.begin()->name!="")
		insert_metric=true;

	index_map_t ind_free, ind_dummy;
	classify_indices(it, ind_free, ind_dummy);
	index_map_t::iterator dit=ind_free.begin();
	index_map_t::iterator prev=ind_free.end();
	dit=ind_dummy.begin();
	prev=dit;
	++dit;
	while(dit!=ind_dummy.end()) {
		if(tree_exact_equal(&kernel.properties, (*dit).first, (*prev).first)) {
			if(insert_metric) { // put indices down and insert an inverse metric
				(*dit).second->fl.parent_rel=str_node::p_sub;
				(*prev).second->fl.parent_rel=str_node::p_sub;
				iterator invmet=tr.append_child(it,str_node(metric.begin()->name));

				// get a new dummy index
				const Indices *dums=kernel.properties.get<Indices>(dit->second, true);
				assert(dums);
				Ex dum=get_dummy(dums, it);

				// relink the indices
				iterator tmpit=tr.append_child(invmet, (*prev).second);
				tmpit->fl.bracket=str_node::b_none;
				tmpit->fl.parent_rel=str_node::p_super;
				tmpit=tr.append_child(invmet, dum.begin());
				tmpit->fl.bracket=str_node::b_none;
				tmpit->fl.parent_rel=str_node::p_super;
				tr.replace_index((*dit).second,dum.begin());

				res=result_t::l_applied;
				}
			else { // raise one index
				if((*dit).second->fl.parent_rel==(*prev).second->fl.parent_rel) {
					if((*dit).second->fl.parent_rel==str_node::p_super) 
						(*prev).second->fl.parent_rel=str_node::p_sub;
					else
						(*prev).second->fl.parent_rel=str_node::p_super;
					}

				res=result_t::l_applied;
				}
			}
		prev=dit;
		++dit;
		}
	return result_t::l_applied;
	}

