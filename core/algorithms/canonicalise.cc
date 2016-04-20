
#include "Cleanup.hh"
#include "Exchange.hh"
#include "algorithms/canonicalise.hh"
#include "modules/xperm_new.h"
#include "properties/Traceless.hh"
#include "properties/Diagonal.hh"
#include "properties/Derivative.hh"
#include "properties/AntiCommuting.hh"

canonicalise::canonicalise(const Kernel& k, Ex& tr)
	: Algorithm(k, tr), reuse_generating_set(false) 
	{
	}

bool canonicalise::can_apply(iterator it) 
	{
	if(*(it->name)!="\\prod")
		return(is_single_term(it));
	
	sibling_iterator sib=tr.begin(it);
	while(sib!=tr.end(it)) {
		// If a factor is a sum or a product, we can only canonicalise
		// if those are scalars, i.e. carry no indices. 
		// FIXME: For the time being, we even forbid dummy indices.
		if(*sib->name=="\\sum" || *sib->name=="\\prod" ) {
			index_map_t ind_dummy, ind_free;
			classify_indices(sib, ind_free, ind_dummy);
			if(ind_free.size()+ind_dummy.size()>0)
				return false;
			else 
				return true;
			}
		++sib;
		}
	return true;
	}

bool canonicalise::remove_traceless_traces(iterator& it)
	{
	// Remove any traces of traceless tensors (this is best done early).
	sibling_iterator facit=tr.begin(it);
	while(facit!=tr.end(it)) {
		const Traceless *trl=kernel.properties.get_composite<Traceless>(facit);
		if(trl) {
			tree_exact_less_mod_prel_obj comp(&kernel.properties);
			std::set<Ex, tree_exact_less_mod_prel_obj> countmap(comp);
			index_iterator indit=begin_index(facit);
			while(indit!=end_index(facit)) {
				if(countmap.find(Ex(indit))==countmap.end()) {
					countmap.insert(Ex(indit));
					++indit;
					}
				else {
					zero(it->multiplier);
					return true;
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
		const Diagonal *dgl=kernel.properties.get_composite<Diagonal>(facit);
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

std::string canonicalise::get_index_set_name(iterator it) const
	{
	const Indices *ind=kernel.properties.get<Indices>(it, true);
	if(ind) {
		return ind->set_name;
// TODO: The logic was once as below, but it is no longer clear to
// me why that would ever make sense.
//		if(ind->parent_name!="") return ind->parent_name;
//		else                     return ind->set_name;
		}
	else return " undeclared";
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
//	std::cerr << "canonicalise at " << Ex(it) << std::endl;
//	std::cerr << is_single_term(it) << std::endl;
//	if(is_single_term(it)==false) {
//		std::cerr << "not acting" << std::endl;
//		return result_t::l_no_action;
//		}

	stopwatch totalsw;
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
	
	// If there are no indices, there is nothing to do here...
	if(total_number_of_indices==0) {
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

	for(auto& dummy: ind_dummy)
		if(ind_dummy.count(dummy.first)>2)
			return result_t::l_no_action;

	for(auto& free: ind_free)
		if(ind_free.count(free.first)>1)
			return result_t::l_no_action;

	result_t res=result_t::l_no_action;

	// Construct the "name to slot" map from the order in ind_free & ind_dummy.
	// Also construct the free and dummy lists.
	// And a map from index number to iterator (for later).
	std::vector<int> vec_perm;
	int              *free_indices=new int[ind_free.size()];
	

	// We need two arrays: one which maps from the order in which slots appear in 
	//	the tensor to the corresponding iterator (this is provided by the standard
	// index routines already), and one which maps from the order in which the indices 
	// appear in the base map to an Ex object (so that we can replace).

	std::vector<Ex::iterator> num_to_it_map(total_number_of_indices);
	std::vector<Ex>           num_to_tree_map;

	// Handle free indices.
	
	index_map_t::iterator sorted_it=ind_free.begin();
	int curr_index=0;
	while(sorted_it!=ind_free.end()) {
		index_position_map_t::iterator ii=ind_pos_free.find(sorted_it->second);
		num_to_it_map[ii->second]=ii->first;
		num_to_tree_map.push_back(Ex(ii->first));
		free_indices[curr_index++]=ii->second+1;
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
		txtout << *(ii->first->name) << " at pos " << ii->second+1 << " " << ii->first->fl.parent_rel << std::endl;
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
		if( ( (separated_by_derivative(ii->first, i2->first,tmp) 
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

	// FIXME: handle 'repeated' sets (numerical indices)
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
	
	// Construct the generating set.

	std::vector<unsigned int> base_here;

	if(!reuse_generating_set || generating_set.size()==0) {
		generating_set.clear();
		// Symmetry of individual tensors.
		sibling_iterator facit=tr.begin(it);
		int curr_pos=0;
		while(facit!=tr.end(it)) {
			const TableauBase *tba=kernel.properties.get_composite<TableauBase>(facit);
			if(tba) {
				unsigned int num_ind=number_of_indices(facit);

				// Add indices to the base. We used to add everything except the last one, but that
				// seems to be the wrong thing to do after the XPERM -> XPERM_EXT upgrade (see Jose's email).
				for(unsigned int kk=0; kk<num_ind; ++kk) 
					base_here.push_back(curr_pos+kk+1);
				
				// loop over tabs
				for(unsigned int ti=0; ti<tba->size(kernel.properties, tr, facit); ++ti) {
					TableauBase::tab_t tmptab=tba->get_tab(kernel.properties, tr,facit,ti);
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
					else if(tmptab.number_of_rows()>0) { // find symmetry under equal-length column exchange
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
	txtout << generating_set.size() << " " << *it->multiplier << std::endl;
#endif
	if(*it->multiplier!=0) {
		// Fill data for the xperm routines.
		int *gs=0;

		if(generating_set.size()>0) {
			gs=new int[generating_set.size()*generating_set[0].size()];
			for(unsigned int i=0; i<generating_set.size(); ++i) {
				for(unsigned int j=0; j<total_number_of_indices+2; ++j) {
					gs[i*(total_number_of_indices+2)+j]=generating_set[i][j];
#ifdef XPERM_DEBUG
					txtout << gs[i*(total_number_of_indices+2)+j] << " ";
#endif				
					}
#ifdef XPERM_DEBUG
				txtout << std::endl;
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

		int  *lengths_of_repeated_sets=new int[1];
		int          *repeated_indices=new int[1];

		lengths_of_repeated_sets[0]=0;
		
#ifdef XPERM_DEBUG
			txtout << "perm:" << std::endl;
			for(unsigned int i=0; i<total_number_of_indices+2; ++i)
			txtout << perm[i] << " "; 
			txtout << std::endl;
			txtout << "base:" << std::endl;
			for(unsigned int i=0; i<base_here.size(); ++i)
			txtout << base[i] << " "; 
			txtout << std::endl;
			txtout << "free indices:" << std::endl;
			for(unsigned int i=0; i<ind_free.size(); ++i)
			txtout << free_indices[i] << " "; 
			txtout << std::endl;
			txtout << "lengths_of_dummy_sets:" << std::endl;
			for(unsigned int i=0; i<dummy_sets.size(); ++i)
				txtout << lengths_of_dummy_sets[i] 
						 << " (metric=" << metric_signatures[i] << ") "; 
			txtout << std::endl;
			txtout << "dummies:" << std::endl;
			for(unsigned int i=0; i<ind_dummy.size(); ++i)
				txtout << dummies[i] << " "; 
			txtout << std::endl;
#endif

		stopwatch sw;
		sw.start();

		// JMM now uses a different convention. 
		int *perm1 = new int[total_number_of_indices+2];
		int *perm2 = new int[total_number_of_indices+2];
		int *free_indices_new_order = new int[ind_free.size()];
		int *dummies_new_order      = new int[ind_dummy.size()];

		inverse(perm, perm1, total_number_of_indices+2);
		for(size_t i=0; i<ind_free.size(); i++) {
			free_indices_new_order[i] = onpoints(free_indices[i], perm1, total_number_of_indices+2);
			}
		for(size_t i=0; i<ind_dummy.size(); i++) {
			dummies_new_order[i] = onpoints(dummies[i], perm1, total_number_of_indices+2);
			}
#ifdef XPERM_DEBUG
		txtout << "perm1: ";
		for(unsigned int i=0; i<total_number_of_indices; ++i) {
			txtout << perm1[i] << " ";
			}
		txtout << std::endl;
#endif
		// Brief reminder of the meaning of the various arrays, using the example in
		// Jose's xPerm paper (not yet updated to reflect the _ext version which allows
		// for multiple dummy sets and repeated (numerical) indices):
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
								 0, //lengths_of_repeated_sets,   // list of lengths of repeated-sets
								 0,                          //    its length
								 0, //repeated_indices,           // list with repeated indices
								 0,                          //    its length
								 perm2);                     // output

		if (perm2[0] != 0) inverse(perm2, cperm, total_number_of_indices+2);
		else copy_list(perm2, cperm, total_number_of_indices+2);

		delete [] dummies_new_order;
		delete [] free_indices_new_order;
		delete [] perm1;
		delete [] perm2;

		sw.stop();
//		txtout << "xperm took " << sw << std::endl;

#ifdef XPERM_DEBUG		
		txtout << "cperm:" << std::endl;
		for(unsigned int i=0; i<total_number_of_indices+2; ++i)
			txtout << cperm[i] << " ";
		txtout << std::endl;
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
					txtout << "putting index " << i+1 << "(" << *num_to_tree_map[i].begin()->name 
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
		}
	
	cleanup_dispatch(kernel, tr, it);

	delete [] free_indices;

	totalsw.stop();
//	txtout << "total canonicalise took " << totalsw << std::endl;
	
	return res;
	}
