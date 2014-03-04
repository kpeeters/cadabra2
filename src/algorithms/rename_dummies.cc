
#include "algorithms/rename_dummies.hh"
#include "properties/Indices.hh"
#include "Exceptions.hh"

rename_dummies::rename_dummies(Kernel& k, exptree& tr)
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
	typedef std::map<exptree, exptree, tree_exact_less_for_indexmap_obj> repmap_t;
	repmap_t    repmap;
	index_map_t added_dummies;

	index_iterator ii=begin_index(st);
	while(ii!=end_index(st)) {
		if(ind_dummy.find(exptree(ii))!=ind_dummy.end()) {
			repmap_t::iterator rmi=repmap.find(exptree(ii));
			if(rmi==repmap.end()) {
				const Indices *dums=kernel.properties.get<Indices>(ii, true);
				if(!dums)
					throw ConsistencyException("No index set for index "+*ii->name+" known.");

				exptree relabel=get_dummy(dums, &ind_free, &ind_free_up, &ind_dummy_up, &added_dummies);
				repmap.insert(repmap_t::value_type(exptree(ii),relabel));
				added_dummies.insert(index_map_t::value_type(relabel, ii));
				index_iterator tmp(ii);
				++tmp;
				tr.replace_index(ii, relabel.begin());
				ii=tmp;
				}
			else {
				index_iterator tmp(ii);
				++tmp;
				tr.replace_index(ii, (*rmi).second.begin());
				ii=tmp;
				}
			}
		else ++ii;
		}

	prod_unwrap_single_term(st);

	expression_modified=true;
	return l_applied;
	}
