
#include "algorithms/rename_dummies.hh"
#include "properties/Indices.hh"
#include "Exceptions.hh"

using namespace cadabra;

rename_dummies::rename_dummies(const Kernel& k, Ex& tr, std::string d1, std::string d2)
	: Algorithm(k, tr), dset1(d1), dset2(d2)
	{
	}

bool rename_dummies::can_apply(iterator st)
	{
	//	std::cerr << "---" << std::endl << Ex(st);

	if(*st->name=="\\equals") {
		// special case: rename all free indices on lhs and rhs.
		// FIXME: add flag to class to disable this when called as rename_dummies.
		return true;
		}

	if(*st->name!="\\prod") // && *st->name!="\\sum")
		if(!is_single_term(st))
			return false;

	//	if(*st->name=="\\prod" && tr.is_head(st)==false && *(tr.parent(st)->name)=="\\sum") return false;
	return true;
	}

Algorithm::result_t rename_dummies::apply(iterator& st)
	{
	result_t res=result_t::l_no_action;

	//	if(*st->name=="\\equals") {
	//
	//		}

	//	std::cerr << Ex(st);
	prod_wrap_single_term(st);
	//	std::cerr << Ex(st);

	// First do a normal classify_indices both downwards and upwards.
	//
	index_map_t ind_free, ind_dummy, ind_free_up, ind_dummy_up;
	classify_indices(st, ind_free, ind_dummy);
	classify_indices_up(st, ind_free_up, ind_dummy_up);

	//	print_classify_indices(std::cerr, st);

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

	// If target set is specified, find a handle to the Indices property
	// with this name.
	const Indices *ind2=0;
	if(dset2!="") {
		auto f2=kernel.properties.pats.begin();
		while(f2!=kernel.properties.pats.end()) {
			ind2 = dynamic_cast<const Indices *>(f2->first);
			if(ind2) {
				if(ind2->set_name==dset2)
					break;
				else ind2=0;
				}
			++f2;
			}
		if(ind2==0)
			throw ConsistencyException("No index set with name `"+dset2+"' known.");
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
				const Indices *dums=kernel.properties.get<Indices>(ii, true);
				if(!dums)
					throw ConsistencyException("No index set for index "+*ii->name+" known.");

				// only rename dummies from dset1.
				if(dset1=="" || dums->set_name==dset1) {
					if(dset2!="") dums=ind2; // replace with dummies from set 2

					Ex relabel=get_dummy(dums, &ind_free, &ind_free_up, &ind_dummy_up, &added_dummies);
					repmap.insert(repmap_t::value_type(Ex(ii),relabel));
					added_dummies.insert(index_map_t::value_type(relabel, ii));
					if(subtree_compare(&kernel.properties, ii, relabel.begin())!=0) {
						res=result_t::l_applied;
						tr.replace_index(ii, relabel.begin(), true);
						}
					}
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

	// Now rename free indices.

	//	if(*st->name=="\\equals") {
	//		auto fit = ind_free.begin();
	//		while(fit!=ind_free.end()) {
	//			std::cerr << "renaming " << fit->first << std::endl;
	//			Ex relabel=get_dummy(dums, &ind_free, &ind_free_up, &ind_dummy_up, &added_dummies);
	//			++fit;
	//			}
	//		std::cerr << "----" << std::endl;
	//		}

	//	std::cerr << Ex(st);
	prod_unwrap_single_term(st);
	//	std::cerr << Ex(st);

	return res;
	}
