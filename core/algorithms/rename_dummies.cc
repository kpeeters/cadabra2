
#include "algorithms/rename_dummies.hh"
#include "properties/Indices.hh"
#include "Exceptions.hh"

rename_dummies::rename_dummies(const Kernel& k, Ex& tr)
	: Algorithm(k, tr)
	{
	}

bool rename_dummies::can_apply(iterator st)
	{
	if(*st->name!="\\prod") 
		if(!is_single_term(st))
			return false;
	return true;
	}

Algorithm::result_t rename_dummies::apply(iterator& st)
	{
	result_t res=result_t::l_no_action;

	prod_wrap_single_term(st);

	// First do a normal classify_indices both downwards and upwards.
	//
	index_map_t ind_free, ind_dummy, ind_free_up, ind_dummy_up;
	classify_indices(st, ind_free, ind_dummy);
	classify_indices_up(st, ind_free_up, ind_dummy_up);
	
	// Run through all indices once more, in order. If an index
	// occurs in the ind_dummy set, and there is no entry in repmap,
	// find the index type and get a new dummy. If the index already
	// occurs in repmap, reuse the new dummy stored there.
	//
	typedef std::map<Ex, Ex, tree_exact_less_for_indexmap_obj> repmap_t;
	repmap_t    repmap;
	index_map_t added_dummies;

	// Store all indices in a map sorted by the name of the parent.
	// FIXME: this is not sufficient, you really need to determine which
	// are common factors in all terms in a sum, and then collect those
	// to the front of the renaming queue.
	std::multimap<nset_t::iterator, index_iterator, nset_it_less> parent_sorted_indices;
	auto ii = begin_index(st);
	while(ii!=end_index(st)) {
		parent_sorted_indices.insert(std::make_pair(tr.parent(iterator(ii))->name, ii));
		++ii;
		}

	auto iim=parent_sorted_indices.begin();
	while(iim!=parent_sorted_indices.end()) {
		ii = iim->second;
		if(ind_dummy.find(Ex(ii))!=ind_dummy.end()) {
			// std::cerr << Ex(ii) << " is dummy " << std::endl;
			repmap_t::iterator rmi=repmap.find(Ex(ii));
			if(rmi==repmap.end()) {
				Ex other_parent_rel(ii);
				if(other_parent_rel.begin()->fl.parent_rel==str_node::p_super) 
					other_parent_rel.begin()->fl.parent_rel=str_node::p_sub;
				else
					other_parent_rel.begin()->fl.parent_rel=str_node::p_super;
				rmi=repmap.find(other_parent_rel);
				}
			if(rmi==repmap.end()) {
				// std::cerr << " not found yet " << std::endl;
				const Indices *dums=kernel.properties.get<Indices>(ii, true);
				if(!dums)
					throw ConsistencyException("No index set for index "+*ii->name+" known.");

				Ex relabel=get_dummy(dums, &ind_free, &ind_free_up, &ind_dummy_up, &added_dummies);
				repmap.insert(repmap_t::value_type(Ex(ii),relabel));
				added_dummies.insert(index_map_t::value_type(relabel, ii));
//				index_iterator tmp(ii);
//				++tmp;
				if(subtree_compare(&kernel.properties, ii, relabel.begin())!=0) {
					res=result_t::l_applied;
					tr.replace_index(ii, relabel.begin(), true);
					}
//				ii=tmp;
				}
			else {
				// std::cerr << "already encountered => " << rmi->second << std::endl;
//				index_iterator tmp(ii);
//				++tmp;
				tr.replace_index(ii, (*rmi).second.begin(), true);
//				ii=tmp;
				}
			}
		++iim;
		}

	prod_unwrap_single_term(st);

	return res;
	}
