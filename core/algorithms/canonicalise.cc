
#include "Cleanup.hh"
#include "Exchange.hh"
#include "Functional.hh"
#include "algorithms/canonicalise.hh"
#include "modules/xperm_new.h"
#include "properties/Trace.hh"
#include "properties/Traceless.hh"
#include "properties/Diagonal.hh"
#include "properties/Derivative.hh"
#include "properties/AntiCommuting.hh"

// #define DEBUG 1
// #define XPERM_DEBUG 1

using namespace cadabra;

canonicalise::canonicalise(const Kernel& k, Ex& tr)
	: Algorithm(k, tr), reuse_generating_set(false)
	{
	}

bool canonicalise::can_apply(iterator it)
	{
	if(*(it->name)!="\\prod")
		if(*(it->name)=="\\pow" || is_single_term(it)==false)
			return false;

	//if(! (tr.is_head(it)==false && *tr.parent(it)->name=="\\pow" && tr.index(it)==0) )

	// Canonicalise requires strict monomial structure: no products which contain
	// sums as factors. Products as factors are ok, they do not lead to multiple
	// identically named free indices.

	auto sum_or_prod = find_in_subtree(tr, it, [this](Ex::iterator tst) {
		if(*tst->name=="\\sum" && number_of_indices(tst)>0) return true;
		return false;
		}, false);
	if(sum_or_prod!=tr.end()) {
#ifdef DEBUG
		std::cerr << "trying to canonicalise nested product/sum " << Ex(it) << " " << Ex(sum_or_prod) << std::endl;
#endif
		return false;
		}

#ifdef DEBUG
	std::cerr << "canonicalise::can_apply: at " << it << std::endl;
#endif
	return true;
	}

bool canonicalise::remove_traceless_traces(iterator& it)
	{
	// Remove any traces of traceless tensors (this is best done early).
	sibling_iterator facit=tr.begin(it);
	while(facit!=tr.end(it)) {
		const Traceless *trl=kernel.properties.get<Traceless>(facit);
		if(trl) {
			unsigned int ihits=0;
			tree_exact_less_mod_prel_obj comp(&kernel.properties);
			std::set<Ex, tree_exact_less_mod_prel_obj> countmap(comp);
			index_iterator indit=begin_index(facit);
			while(indit!=end_index(facit)) {
				bool incremented_now=false;
				auto ind=kernel.properties.get<Indices>(indit, true);
				if(ind) {
					// The indexs need to be in the set for which the object is
					// traceless (if specified, otherwise accept all).
					if(trl->index_set_names.find(ind->set_name)!=trl->index_set_names.end() || trl->index_set_names.size()==0) {
						incremented_now=true;
						++ihits;
						}
					}
				else incremented_now=true;
				// Having no name is treated as having the right name
				if(countmap.find(Ex(indit))==countmap.end()) {
					countmap.insert(Ex(indit));
					}
				else if(incremented_now) {
					zero(it->multiplier);
					return true;
					}
				++indit;
				}
			iterator parent=it;
			if (tr.number_of_children(it)==1 && !tr.is_head(it)) parent=tr.parent(it);
			const Trace *trace=kernel.properties.get<Trace>(parent);
			if(trace) {
				int tmp;
				auto impi=kernel.properties.get_with_pattern<ImplicitIndex>(facit, tmp, "");
				if(impi.first->explicit_form.size()>0) {
					// Does the explicit form have two more indices of the right type?
					Ex::iterator eform=impi.first->explicit_form.begin();
					unsigned int ehits=0;
					indit=begin_index(eform);
					while(indit!=end_index(eform)) {
						auto ind=kernel.properties.get<Indices>(indit, true);
						if(trl->index_set_names.find(ind->set_name)!=trl->index_set_names.end() && ind->set_name==trace->index_set_name) ++ehits;
						if(ehits - ihits > 1) {
							zero(it->multiplier);
							return true;
							}
						++indit;
						}
					}
				}
			}
		++facit;
		}
	return false;
	}

bool canonicalise::remove_vanishing_numericals(iterator& it)
	{
	// Remove Diagonal objects with numerical indices which are not all the same.
	sibling_iterator facit=tr.begin(it);
	while(facit!=tr.end(it)) {
		const Diagonal *dgl=kernel.properties.get<Diagonal>(facit);
		if(dgl) {
			index_iterator indit=begin_index(facit);
			if(indit->is_rational()) {
				index_iterator indit2=indit;
				++indit2;
				while(indit2!=end_index(facit)) {
					if(indit2->is_rational()==false)
						break;
					if(indit2->multiplier!=indit->multiplier) {
						zero(it->multiplier);
						return true;
						}
					++indit2;
					}
				}
			}
		++facit;
		}
	return false;
	}

Indices::position_t canonicalise::position_type(iterator it) const
	{
	const Indices *ind=kernel.properties.get<Indices>(it, true);
	if(ind)
		return ind->position_type;
	return Indices::free;
	}

bool canonicalise::only_one_on_derivative(iterator i1, iterator i2) const
	{
	int num=0;
	iterator p1=tr.parent(i1);
	const Derivative *der1=kernel.properties.get<Derivative>(p1);
	if(der1) ++num;

	iterator p2=tr.parent(i2);
	const Derivative *der2=kernel.properties.get<Derivative>(p2);
	if(der2) ++num;

	if(num==1) return true;
	else return false;
	}

Algorithm::result_t canonicalise::apply(iterator& it)
	{
#ifdef DEBUG
	std::cerr << "canonicalise at " << it << std::endl;
#endif
	// std::cerr << is_single_term(it) << std::endl;

	Stopwatch totalsw;
	totalsw.start();
	prod_wrap_single_term(it);

	if(remove_traceless_traces(it)) {
		cleanup_dispatch(kernel, tr, it);
		return result_t::l_applied;
		}

	if(remove_vanishing_numericals(it)) {
		cleanup_dispatch(kernel, tr, it);
		return result_t::l_applied;
		}


	// Now the real thing...
	index_map_t ind_free, ind_dummy;
	classify_indices(it, ind_free, ind_dummy);
	index_position_map_t ind_pos_free, ind_pos_dummy;
	fill_index_position_map(it, ind_free, ind_pos_free);
	fill_index_position_map(it, ind_dummy, ind_pos_dummy);
	const unsigned int total_number_of_indices=ind_free.size()+ind_dummy.size();

#ifdef DEBUG
	//	std::cerr << "free index position map:\n";
	//	for(auto& ip: ind_pos_free)
	//		std::cerr << Ex(ip.first) << " @ " << ip.second << std::endl;;
#endif

	// If there are no indices, there is nothing to do here...
	if(total_number_of_indices==0) {
#ifdef DEBUG
		std::cerr << "no indices on " << Ex(it) << std::endl;
#endif
		prod_unwrap_single_term(it);
		return result_t::l_no_action;
		}

	// We currently do not handle situations in which one of the
	// factors in a product is a sum. To be precise, we do not handle
	// situations in which one or both of the indices in a dummy
	// pair appear multiple times (in the different terms of the sum).
	// Ditto for free indices; these need to sit in one particular
	// place in the tree, not in multiple. For these situations, we
	// will bail out here, but the user can always distribute and then
	// canonicalise.

#ifdef DEBUG
	//	std::cerr << "dummies:\n";
	//	for(auto& dummy: ind_dummy)
	//		std::cerr << dummy.first;
	//	std::cerr << "free:\n";
	//	for(auto& fr: ind_free)
	//		std::cerr << fr.first;
#endif

	for(auto& dummy: ind_dummy)
		if(ind_dummy.count(dummy.first)>2)
			return result_t::l_no_action;

	// PROGRESS
	//	for(auto& free: ind_free)
	//		if(ind_free.count(free.first)>1) {
	//			std::cerr << "bailing out" << std::endl;
	//			return result_t::l_no_action;
	//			}

	result_t res=result_t::l_no_action;

	// Construct the "name to slot" map from the order in ind_free & ind_dummy.
	// Also construct the free and dummy lists.
	// And a map from index number to iterator (for later).
	std::vector<int> vec_perm;


	// We need two arrays: one which maps from the order in which slots appear in
	//	the tensor to the corresponding iterator (this is provided by the standard
	// index routines already), and one which maps from the order in which the indices
	// appear in the base map to an Ex object (so that we can replace).

	std::vector<Ex::iterator> num_to_it_map(total_number_of_indices);
	std::vector<Ex>           num_to_tree_map;

#ifdef DEBUG
	std::cerr << "indices:" << std::endl;
	auto ii=begin_index(it);
	while(ii!=end_index(it)) {
		std::cerr << ii << std::endl;
		++ii;
		}
#endif


	// Handle free indices.

#ifdef DEBUG
	std::cerr << "found " << ind_free.size() << " free indices" << std::endl;
#endif
	index_map_t::iterator sorted_it=ind_free.begin();
	int curr_index=0;
	while(sorted_it!=ind_free.end()) {
		index_position_map_t::iterator ii=ind_pos_free.find(sorted_it->second);
		num_to_it_map.at(ii->second)=ii->first;
		num_to_tree_map.push_back(Ex(ii->first));
#ifdef DEBUG
		std::cerr << sorted_it->second << " free at pos " << ii->second+1 << std::endl;
#endif
		vec_perm.push_back(ii->second+1);

		++sorted_it;
		}
	curr_index=0;


	// Handle dummy indices
	// In order to ensure that dummy indices from different index types do not
	// get mixed up, we need to collect information about the types of all
	// dummy indices.
	// The key in the maps below is the set_name of the Indices property, or the
	// set_name of the parent if applicable. If none, it will be ' undeclared'.

	typedef std::map<std::string, std::vector<int> > dummy_set_t;
	dummy_set_t dummy_sets;
#ifdef DEBUG
	std::cerr << "found " << ind_dummy.size() << " dummies" << std::endl;
#endif
	sorted_it=ind_dummy.begin();
	while(sorted_it!=ind_dummy.end()) {
		// We insert the dummy indices in pairs (canonicalise only acts on
		// expressions which have dummies coming in doublets, not the more
		// general cadabra dummy concept).
		// The lower index come first, and then the upper index.

		index_position_map_t::const_iterator ii=ind_pos_dummy.find(sorted_it->second);
		index_map_t::const_iterator          next_it=sorted_it;
		++next_it;
		index_position_map_t::const_iterator i2=ind_pos_dummy.find(next_it->second);

#ifdef XPERM_DEBUG
		std::cerr << *(ii->first->name) << " at pos " << ii->second+1 << " " << ii->first->fl.parent_rel << std::endl;
#endif

		switch(ii->first->fl.parent_rel) {
			case str_node::p_super:
			case str_node::p_none:
				//				vec_perm.push_back(ii->second+1);
				//				vec_perm.push_back(i2->second+1);
				//				num_to_tree_map.push_back(Ex(ii->first));
				//				num_to_tree_map.push_back(Ex(i2->first));
				break;
			case str_node::p_sub:
				std::swap(ii, i2);
				//				vec_perm.push_back(i2->second+1);
				//				vec_perm.push_back(ii->second+1);
				//				num_to_tree_map.push_back(Ex(i2->first));
				//				num_to_tree_map.push_back(Ex(ii->first));
				break;
			default:
				break;
			}

		vec_perm.push_back(ii->second+1);
		vec_perm.push_back(i2->second+1);
		num_to_tree_map.push_back(Ex(ii->first));
		num_to_tree_map.push_back(Ex(i2->first));

		num_to_it_map[ii->second]=ii->first;
		num_to_it_map[i2->second]=i2->first;

		// If the indices are not in canonical order and they are separated
		// by a Derivative, we cannot raise/lower, so they should stay in this order.
		// Have to do this by putting those indices in a different set and then
		// setting the metric flag to 0. Ditto when only one index is on a derivative
		// (canonicalising usually makes the expression uglier in that case).
		iterator tmp;
		if( ( (separated_by_derivative(tr.parent(ii->first), tr.parent(i2->first),tmp)
		       || only_one_on_derivative(ii->first, i2->first) )
		      && position_type(ii->first)==Indices::fixed ) ||
		      position_type(ii->first)==Indices::independent ) {
			dummy_sets[" NR "+get_index_set_name(ii->first)].push_back(ii->second+1);
			dummy_sets[" NR "+get_index_set_name(i2->first)].push_back(i2->second+1);
			}
		else {
			if( kernel.properties.get<AntiCommuting>(ii->first, true) != 0 ) {
				dummy_sets[" AC "+get_index_set_name(ii->first)].push_back(ii->second+1);
				dummy_sets[" AC "+get_index_set_name(i2->first)].push_back(i2->second+1);
				}
			else {
				dummy_sets[get_index_set_name(ii->first)].push_back(ii->second+1);
				dummy_sets[get_index_set_name(i2->first)].push_back(i2->second+1);
				}
			}

		++sorted_it;
		++sorted_it;
		}

	// FIXME: kludge to handle numerical indices; should be done through lookup
	// in Integer properties. This one does NOT work when there is more than
	// one index set; we would need more clever logic to figure out which
	// index type the numerical index corresponds to.
	//	debugout << index_sets.size() << std::endl;
	//	if(index_sets.size()==1) {
	//		for(unsigned int kk=0; kk<indexpos_to_indextype.size(); ++kk)
	//			if(indexpos_to_indextype[kk]=="numerical")
	//				indexpos_to_indextype[kk]=index_sets.begin()->first;
	//		}
	//

	// TRACE: remove later THIS DOES NOT SOLVE THE PROBLEM, ITERATORS ABOVE SEEM TO BE WRONG.
	//	prod_unwrap_single_term(it);
	//	return result_t::l_no_action;


	// Construct the generating set.

	std::vector<unsigned int> base_here;

	if(!reuse_generating_set || generating_set.size()==0) {
		generating_set.clear();
		// Symmetry of individual tensors.
		sibling_iterator facit=tr.begin(it);
		int curr_pos=0;
		while(facit!=tr.end(it)) {
			const TableauBase *tba=kernel.properties.get<TableauBase>(facit);
			// std::cerr << Ex(facit) << " has tableaubase " << tba << std::endl;
			if(tba) {
				unsigned int num_ind=number_of_indices(facit);
#ifdef XPERM_DEBUG
				std::cerr << "factor " << *facit->name << " has " << num_ind << " indices" << std::endl;
#endif

				// Add indices to the base. We used to add everything except the last one, but that
				// seems to be the wrong thing to do after the XPERM -> XPERM_EXT upgrade (see Jose's email).
				for(unsigned int kk=0; kk<num_ind; ++kk)
					base_here.push_back(curr_pos+kk+1);

				// loop over tabs
				for(unsigned int ti=0; ti<tba->size(kernel.properties, tr, facit); ++ti) {
					TableauBase::tab_t tmptab=tba->get_tab(kernel.properties, tr,facit,ti);
					// std::cerr << tmptab << std::endl;
					if(tmptab.number_of_rows()>0) {
						for(unsigned int col=0; col<tmptab.row_size(0); ++col) { // anti-symmetry in all inds in a col
							if(tmptab.column_size(col)>1) {
								// all pairs NEW: SGS
								for(unsigned int indnum1=0; indnum1<tmptab.column_size(col)-1; ++indnum1) {
									//								for(unsigned int indnum2=indnum1+1; indnum2<tmptab.column_size(col); ++indnum2) {
									std::vector<int> permute(total_number_of_indices+2);
									for(unsigned int kk=0; kk<permute.size(); ++kk)
										permute[kk]=kk+1;
									std::swap(permute[tmptab(indnum1,col)+curr_pos],
									          permute[tmptab(indnum1+1,col)+curr_pos]);
									std::swap(permute[total_number_of_indices+1],
									          permute[total_number_of_indices]); // anti-symmetry
									generating_set.push_back(permute);
									}
								}
							}
						}
					if(tmptab.number_of_rows()==1 && tmptab.row_size(0)>1) { // symmetry, if all cols of size 1
						// all pairs
						for(unsigned int indnum1=0; indnum1<tmptab.row_size(0)-1; ++indnum1) {
							//						for(unsigned int indnum2=indnum1+1; indnum2<tmptab.row_size(0); ++indnum2) {
							std::vector<int> permute(total_number_of_indices+2);
							for(unsigned int kk=0; kk<permute.size(); ++kk)
								permute[kk]=kk+1;
							std::swap(permute[tmptab(0,indnum1)+curr_pos],
							          permute[tmptab(0,indnum1+1)+curr_pos]);
							generating_set.push_back(permute);
							}
						}
					else if(tmptab.number_of_rows()>0) {   // find symmetry under equal-length column exchange
						unsigned int column_height=tmptab.column_size(0);
						unsigned int this_set_start=0;
						for(unsigned int col=1; col<=tmptab.row_size(0); ++col) {
							if(col==tmptab.row_size(0) || column_height!=tmptab.column_size(col)) {
								if(col-this_set_start>1) {
									// two or more equal-length columns found, make generating set
									for(unsigned int col1=this_set_start; col1+1<=col-1; ++col1) {
										//									for(unsigned int col2=this_set_start+1; col2<col; ++col2) {
										std::vector<int> permute(total_number_of_indices+2);
										for(unsigned int kk=0; kk<permute.size(); ++kk)
											permute[kk]=kk+1;
										for(unsigned int row=0; row<column_height; ++row) {
											//										txtout << row << " " << col1 << std::endl;
											std::swap(permute[tmptab(row,col1)+curr_pos],
											          permute[tmptab(row,col1+1)+curr_pos]);
											}
										generating_set.push_back(permute);
										}
									}
								this_set_start=col;
								if(col<tmptab.row_size(0))
									column_height=tmptab.column_size(col);
								}
							}
						}
					}
				//					txtout << "loop over tabs done" << std::endl;
				curr_pos+=num_ind;
				}
			else {
				unsigned int num_ind=number_of_indices(facit);
				if(num_ind==1)
					base_here.push_back(curr_pos+1);
				else {
					for(unsigned int kk=0; kk<num_ind; ++kk)
						base_here.push_back(curr_pos+kk+1);
					}
				curr_pos+=number_of_indices(facit); // even if tba=0, this factor may contain indices
				}
			++facit;
			}
		// Symmetry under tensor exchange.
		if(exchange::get_node_gs(kernel.properties, tr, it, generating_set)==false) {
			zero(it->multiplier);
			res=result_t::l_applied;
			}
		}
	// End of construction of generating set.

#ifdef XPERM_DEBUG
	std::cerr << "generating set size = " << generating_set.size() << " multiplier " << *it->multiplier << std::endl;
#endif

	// Even if generating_set.size()==0 we still need to continue,
	// because we still need to do simple index relabelling.
	//	if(generating_set.size()==0) {
	//		prod_unwrap_single_term(it);
	//		return result_t::l_no_action;
	//		}

	if(*it->multiplier!=0) {
		// Fill data for the xperm routines.
		int *gs=0;

		if(generating_set.size()>0) {
			gs=new int[generating_set.size()*generating_set[0].size()];
			for(unsigned int i=0; i<generating_set.size(); ++i) {
				for(unsigned int j=0; j<total_number_of_indices+2; ++j) {
					gs[i*(total_number_of_indices+2)+j]=generating_set[i][j];
#ifdef XPERM_DEBUG
					std::cerr << gs[i*(total_number_of_indices+2)+j] << " ";
#endif
					}
#ifdef XPERM_DEBUG
				std::cerr << std::endl;
#endif
				}
			}

		// Setup the arrays for xperm from our own data structures.

		int    *base=new int[base_here.size()];
		int    *perm=new int[total_number_of_indices+2];
		int   *cperm=new int[total_number_of_indices+2];

		for(unsigned int i=0; i<base_here.size(); ++i)
			base[i]=base_here[i];
		assert(vec_perm.size()==total_number_of_indices);
		for(unsigned int i=0; i<total_number_of_indices; ++i)
			perm[i]=vec_perm[i];
		perm[total_number_of_indices]=total_number_of_indices+1;
		perm[total_number_of_indices+1]=total_number_of_indices+2;

		int  *lengths_of_dummy_sets=new int[dummy_sets.size()];
		int  *dummies              =new int[ind_dummy.size()];
		int  *metric_signatures    =new int[dummy_sets.size()];
		int  dsi=0;
		int  cdi=0;
		dummy_set_t::iterator ds=dummy_sets.begin();
		while(ds!=dummy_sets.end()) {
			lengths_of_dummy_sets[dsi]=ds->second.size();
			for(unsigned int k=0; k<ds->second.size(); ++k)
				dummies[cdi++]=(ds->second)[k];
			if(ds->first.substr(0,4)==" NR ")
				metric_signatures[dsi]=0;
			else {
				if(ds->first.substr(0,4)==" AC ")
					metric_signatures[dsi]=-1;
				else
					metric_signatures[dsi]=1;
				}
			++ds;
			++dsi;
			}

		// Setup repeated sets. These are free indices which appear more
		// than once (so these are coordinates or integers). Take them out
		// of the free set and store them in the repeated sets.

#ifdef DEBUG
		std::cerr << "creating repeated sets" << std::endl;
#endif
		std::vector<int>          ind_repeated_lengths;
		std::vector<Ex::iterator> ind_repeated;
		auto fi = ind_free.begin();
		auto prev=fi;
		if(fi!=ind_free.end()) {
			++fi;
			while(fi!=ind_free.end()) {
				int len=1;
				while(fi!=ind_free.end() && fi->first==prev->first) {
					if(len==1)
						ind_repeated.push_back(prev->second);
					ind_repeated.push_back(fi->second);
					++len;
					++fi;
					}
				if(len!=1) {
					ind_free.erase(prev, fi);
					ind_repeated_lengths.push_back(len);
					}
				if(fi!=ind_free.end()) {
					prev=fi;
					++fi;
					}
				}
			}

		// free_indices stores a list of index slots which contain a free
		// index.
		int              *free_indices=new int[ind_free.size()];
		sorted_it=ind_free.begin();
		curr_index=0;
		while(sorted_it!=ind_free.end()) {
			index_position_map_t::iterator ii=ind_pos_free.find(sorted_it->second);
			free_indices[curr_index++]=ii->second+1;
			++sorted_it;
			}

		//		std::cerr << "repeated sets:\n";
		//		for(auto& f: ind_repeated)
		//			std::cerr << *(f->multiplier) << " ";
		//		std::cerr << "\nrepeated lengths:\n";
		//		for(auto& i: ind_repeated_lengths)
		//			std::cerr << i << " ";
		//		std::cerr << std::endl;

		int *repeated_indices         = new int[ind_repeated.size()];
		int *lengths_of_repeated_sets = new int[ind_repeated_lengths.size()];

		// repeated_indices contains a list of slots which contain repeated indices.
		for(size_t i=0; i<ind_repeated.size(); ++i) {
			auto pos=ind_pos_free.find(ind_repeated[i]);
			repeated_indices[i]=pos->second+1;
			}
		for(size_t i=0; i<ind_repeated_lengths.size(); ++i)
			lengths_of_repeated_sets[i]=ind_repeated_lengths[i];

#ifdef XPERM_DEBUG
		std::cerr << "perm:" << std::endl;
		for(unsigned int i=0; i<total_number_of_indices+2; ++i)
			std::cerr << perm[i] << " ";
		std::cerr << std::endl;
		std::cerr << "base:" << std::endl;
		for(unsigned int i=0; i<base_here.size(); ++i)
			std::cerr << base[i] << " ";
		std::cerr << std::endl;
		std::cerr << "free indices in slots:" << std::endl;
		for(unsigned int i=0; i<ind_free.size(); ++i)
			std::cerr << free_indices[i] << " ";
		std::cerr << std::endl;
		std::cerr << "lengths_of_dummy_sets:" << std::endl;
		for(unsigned int i=0; i<dummy_sets.size(); ++i)
			std::cerr << lengths_of_dummy_sets[i]
			          << " (metric=" << metric_signatures[i] << ") ";
		std::cerr << std::endl;
		std::cerr << "dummies in slots:" << std::endl;
		for(unsigned int i=0; i<ind_dummy.size(); ++i)
			std::cerr << dummies[i] << " ";
		std::cerr << std::endl;
		std::cerr << "lengths_of_repeated_sets:" << std::endl;
		for(unsigned int i=0; i<ind_repeated_lengths.size(); ++i)
			std::cerr << lengths_of_repeated_sets[i];
		std::cerr << std::endl;
		std::cerr << "repeated indices in slots:" << std::endl;
		for(unsigned int i=0; i<ind_repeated.size(); ++i)
			std::cerr << repeated_indices[i];
		std::cerr << std::endl;
#endif

		Stopwatch sw;
		sw.start();

		// JMM now uses a different convention.
		int *perm1 = new int[total_number_of_indices+2];
		int *perm2 = new int[total_number_of_indices+2];
		int *free_indices_new_order = new int[ind_free.size()];
		int *dummies_new_order      = new int[ind_dummy.size()];
		int *repeated_new_order     = new int[ind_repeated.size()];

		inverse(perm, perm1, total_number_of_indices+2);
		for(size_t i=0; i<ind_free.size(); i++) {
			free_indices_new_order[i] = onpoints(free_indices[i], perm1, total_number_of_indices+2);
			}
		for(size_t i=0; i<ind_dummy.size(); i++) {
			dummies_new_order[i] = onpoints(dummies[i], perm1, total_number_of_indices+2);
			}
		for(size_t i=0; i<ind_repeated.size(); i++) {
			repeated_new_order[i] = onpoints(repeated_indices[i], perm1, total_number_of_indices+2);
			}
#ifdef XPERM_DEBUG
		std::cerr << "perm1: ";
		for(unsigned int i=0; i<total_number_of_indices; ++i) {
			std::cerr << perm1[i] << " ";
			}
		std::cerr << std::endl;
#endif
		// Brief reminder of the meaning of the various arrays, using the example in
		// Jose's xPerm paper (not yet updated to reflect the _ext version which allows
		// for multiple dummy sets; see below for repeated (numerical) indices):
		//
		// expression        = R_{b}^{1d1} R_{c}^{bac}
		//
		// sorted_index_set  = {a,d,b,-b,c,-c,1,1}
		// base              = {1,2,3, 4,5, 6,7,8}
		// dummies           = {3,4,5,6}
		//    sorted_index_set[3] and [4] is a dummy pair
		//    sorted_index_set[5] and [6] is a dummy pair
		//    these can just be in sorted order, i.e. only referring
		//    to the sorted_index_set.
		//
		// frees             = {1,2}
		//    sorted_index_set[1] and [2] are free indices
		//
		// dummies_new_order = {dn[1], dn[2], ...}
		//    slot dn[1] and dn[2] form a dummy pair ?
		//
		// free_indices_new_order = {...}
		//    these slots contain free indices ?
		//
		// perm = {4,7,2,8,6,3,1,5,9,10}
		//    1st slot contains sorted_index_set[4] ( = -b )
		//    2nd slot contains sorted_index_set[7] ( = 1 )
		//    etc.
		//
		// perm1 = {p1[1], p1[2], ...}
		//
		//    1st index sits in slot p1[1] ?
		//
		// cperm = {1,3,4,5,2,7,6,8,9,10}
		//
		//    1st slot gets sorted_index_set[1] ( = a )
		//    2nd slot gets sorted_index_set[3] ( = b )
		//    ...
		//
		// for the repeated set logic:
		// (in both perm and free, numbers refer to index names (each index name has a separate number
		// even if it occurs multiple times).
		//
		// R_{3 3 m 3}
		//    perm: 6
		//    2 3 1 4 5 6  (slot 3 gets index name 1, slot 1 index 2, slot 3 index 3, slot 4 index 4)
		//    base: 4
		//    1 2 3 4
		//    free: 1
		//    1            (index name 1 is free)
		//    number of repes: 3
		//    2 3 4        (index names 2 to 4 are repeated);

		// R_{3 m 3 3}
		//    perm: 6
		//    2 1 3 4 5 6   (slot 2 gets index name 1, ...)
		//    base: 4
		//    1 2 3 4       (always the same)
		//    free: 1
		//    1             (index name 1 is free)
		//    number of repes: 3
		//    2 3 4         (2nd, 3rd and 4th index names are repeated: index 1, 3 and 4)


		canonical_perm_ext(perm1,                       // permutation to be canonicalised
		                   total_number_of_indices+2,  // degree (+2 for the overall sign)
		                   1,                          // is this a strong generating set?
		                   base,                       // base for the strong generating set
		                   base_here.size(),           //    its length
		                   gs,                         // generating set
		                   generating_set.size(),      //    its size
		                   free_indices_new_order,     // free indices
		                   ind_free.size(),            // number of free indices
		                   lengths_of_dummy_sets,      // list of lengths of dummy sets
		                   dummy_sets.size(),          //    its length
		                   dummies_new_order,          // list with pairs of dummies
		                   ind_dummy.size(),           //    its length
		                   metric_signatures,          // list of symmetries of metric
		                   lengths_of_repeated_sets,   // list of lengths of repeated-sets
		                   ind_repeated_lengths.size(),//    its length
		                   repeated_new_order,         // list with repeated indices
		                   ind_repeated.size(),        //    its length
		                   perm2);                     // output

		if (perm2[0] != 0) inverse(perm2, cperm, total_number_of_indices+2);
		else copy_list(perm2, cperm, total_number_of_indices+2);

		delete [] dummies_new_order;
		delete [] free_indices_new_order;
		delete [] repeated_new_order;
		delete [] perm1;
		delete [] perm2;

		sw.stop();
		//		std::cerr << "xperm took " << sw << std::endl;

#ifdef XPERM_DEBUG
		std::cerr << "cperm:" << std::endl;
		for(unsigned int i=0; i<total_number_of_indices+2; ++i)
			std::cerr << cperm[i] << " ";
		std::cerr << std::endl;
#endif

		if(cperm[0]!=0) {
			bool has_changed=false;
			for(unsigned int i=0; i<total_number_of_indices+1; ++i) {
				if(perm[i]!=cperm[i]) {
					has_changed=true;
					break;
					}
				}
			if(has_changed) {
				if(static_cast<unsigned int>(cperm[total_number_of_indices+1])==total_number_of_indices+1) {
					flip_sign(it->multiplier);
					}
				res = result_t::l_applied;

				for(unsigned int i=0; i<total_number_of_indices; ++i) {
					// In the new final permutation, e.g.
					//
					// 1 5 6 8 7 2 3 4 10 9
					//
					// we place first the first index (m), which goes to the first slot. Then
					// we put n, which can only go the fifth slot. Then we put p, which can go
					// to 6,7,8, so that it goes to 6. Then we put r (not q), which can go to 7
					// and 8, and so we put it at 7, etc.

#ifdef XPERM_DEBUG
					std::cerr << "putting index " << i+1 << "(" << *num_to_tree_map[i].begin()->name
					          << ", " << num_to_tree_map[i].begin()->fl.parent_rel
					          << ") in slot " << cperm[i] << std::endl;
#endif

					iterator ri = tr.replace_index(num_to_it_map[cperm[i]-1], num_to_tree_map[i].begin());
					//					assert(ri->fl.parent_rel==num_to_tree_map[i].begin()->fl.parent_rel);
					ri->fl.parent_rel=num_to_tree_map[i].begin()->fl.parent_rel;
					}
				}
			}
		else {
			zero(it->multiplier);
			res = result_t::l_applied;
			}

		if(gs)
			delete [] gs;
		delete [] base;

		delete [] repeated_indices;
		delete [] lengths_of_repeated_sets;
		delete [] metric_signatures;
		delete [] lengths_of_dummy_sets;
		delete [] dummies;
		delete [] cperm;
		delete [] perm;
		delete [] free_indices;
		}
#ifdef DEBUG
	std::cerr << "=====\n";
#endif

	cleanup_dispatch(kernel, tr, it);


	totalsw.stop();
	//	std::cerr << "total canonicalise took " << totalsw << std::endl;

	return res;
	}
