
#include "IndexClassifier.hh"
#include "IndexIterator.hh"
#include "Exceptions.hh"
#include "Kernel.hh"
#include "properties/Symbol.hh"
#include "properties/Coordinate.hh"
#include "properties/IndexInherit.hh"
#include <sstream>

using namespace cadabra;

IndexClassifier::IndexClassifier(const Kernel& k)
   : kernel(k)
   {
   }

// For each iterator in the original map, find the sequential position of the index.
// That is, the index 'd' has position '3' in A_{a b} C_{c} D_{d}.
// WARNING: expensive operation.
//
void IndexClassifier::fill_index_position_map(Ex::iterator prodnode, const index_map_t& im, index_position_map_t& ipm) const
	{
	ipm.clear();
	index_map_t::const_iterator imit=im.begin();
	while(imit!=im.end()) {
		int current_pos=0;
		bool found=false;
		index_iterator indexit=index_iterator::begin(kernel.properties, prodnode);
		while(indexit!=index_iterator::end(kernel.properties, prodnode)) {
			if(imit->second==(Ex::iterator)(indexit)) {
				ipm.insert(index_position_map_t::value_type(imit->second, current_pos));
				found=true;
				break;
				}
			++current_pos;
			++indexit;
			}
		if(!found) 
			throw ConsistencyException("Internal error in fill_index_position_map; cannot find index "
											+ *(imit->first.begin()->name)+".");
		++imit;
		}
	}

void IndexClassifier::fill_map(index_map_t& mp, Ex::sibling_iterator st, Ex::sibling_iterator nd) const
	{
	while(st!=nd) {
		mp.insert(index_map_t::value_type(Ex(st), Ex::iterator(st)));
		++st;
		}
	}

// Determine those indices in 'two' which have a name which is identical to
// an index name occurring in 'one'. Store these indices of 'two' in target.
// If 'move_out' is true, instead move both the indices in 'one' and 'two' 
// (i.e. move instead of copy, and also store the 'one' index).
//
// One exception: numerical, coordinate and symbol indices are always kept in 'one'.
//
void IndexClassifier::determine_intersection(index_map_t& one, index_map_t& two, index_map_t& target, bool move_out)  const
	{
	index_map_t::iterator it1=one.begin();
	while(it1!=one.end()) {
		const Coordinate *cdn=kernel.properties.get<Coordinate>(it1->second, true);
		const Symbol     *smb=Symbol::get(kernel.properties, it1->second, true);
		if(it1->second->is_integer()==false && !cdn && !smb) {
			bool move_this_one=false;
			index_map_t::iterator it2=two.begin();
			while(it2!=two.end()) {
				if(tree_exact_equal(&kernel.properties, (*it1).first,(*it2).first,1,true,-2,true)) {
//					const Indices *ind=kernel.properties.get<Indices>(it1->second);
//					if(ind && ind->position_type==Indices::fixed && it1->second->fl.parent_rel==it2->second->fl.parent_rel) {
//						std::cerr << tr << std::endl;
//						throw ConsistencyException("Fixed index pair with two upper or two lower indices "+ *it1->second->name + " found.");
//						}
					target.insert((*it2));
					if(move_out) {
						index_map_t::iterator nxt=it2;
						++nxt;
						two.erase(it2);
						it2=nxt;
						move_this_one=true;
						}
					else ++it2;
					}
				else ++it2;
				}
			Ex the_key=(*it1).first;
			if(move_this_one && move_out) {
				index_map_t::iterator nxt=it1;
				++nxt;
				target.insert(*it1);
				one.erase(it1);
				it1=nxt;
				}
			else ++it1;
			// skip all indices in two with the same name
			while(it1!=one.end() && tree_exact_equal(&kernel.properties, (*it1).first,the_key,1,true,-2,true)) {
				if(move_this_one && move_out) {
					index_map_t::iterator nxt=it1;
					++nxt;
					target.insert(*it1);
					one.erase(it1);
					it1=nxt;
					}
				else ++it1;
				}
			}
		else ++it1;
		}
	}

IndexClassifier::index_map_t::iterator IndexClassifier::find_modulo_parent_rel(Ex::iterator it, index_map_t& imap)  const
	{
	auto fnd=imap.find(it);
	if(fnd==imap.end()) {
		it->flip_parent_rel();
		fnd=imap.find(it);
		it->flip_parent_rel();
		return fnd;
		}
	return fnd;
	}

// Directly add an index to the free/dummy sets, as appropriate (only add if this really is an 
// index!)

void IndexClassifier::classify_add_index(Ex::iterator it, index_map_t& ind_free, index_map_t& ind_dummy) const
	{
	if((it->fl.parent_rel==str_node::p_sub || it->fl.parent_rel==str_node::p_super) &&
		it->fl.bracket==str_node::b_none /* && it->is_integer()==false */) {
		const Coordinate *cdn=kernel.properties.get<Coordinate>(it, true);
		const Symbol     *smb=Symbol::get(kernel.properties, it, true);
		 if(it->is_integer() || cdn || smb)
			  ind_free.insert(index_map_t::value_type(Ex(it), it));
		 else {
			 index_map_t::iterator fnd=find_modulo_parent_rel(it, ind_free);
			 if(fnd!=ind_free.end()) {
				 // std::cerr << "found in free indices" << std::endl;
 				 // check consistency: one up and one down if index position is fixed.
				 const Indices *ind=kernel.properties.get<Indices>(it);
				 if(ind && ind->position_type==Indices::fixed && it->fl.parent_rel==fnd->second->fl.parent_rel)  {
					 throw ConsistencyException("Fixed index pair with two upper or two lower indices found.");
					 }
				 ind_dummy.insert(*fnd);
				 ind_dummy.insert(index_map_t::value_type(Ex(it), it));
				 ind_free.erase(fnd);
				 }
			 else {
				 // std::cerr << "not yet found; after insertion" << std::endl;
				 if(ind_dummy.count(it)>0) {
					 throw ConsistencyException("Triple index occurred.");
					 }
				 ind_free.insert(index_map_t::value_type(Ex(it), it));
				 // for(auto& n: ind_free)
				 //	 std::cerr << n.first << std::endl;
				 }
			 }
		 }
	}

// This classifies indices bottom-up, that is, given a node, it goes up the tree to find
// all free and dummy indices in the product in which this node would end up if a full
// distribute would be done on the entire expression. 
void IndexClassifier::classify_indices_up(Ex::iterator it, index_map_t& ind_free, index_map_t& ind_dummy)  const
	{
	loopie:
	if(Ex::is_head(it)) return;
	Ex::iterator par=Ex::parent(it);
//	if(Ex::is_valid(par)==false || par==tr.end()) { // reached the top
//		return;
//		}
	const IndexInherit *inh=kernel.properties.get<IndexInherit>(par);

	if(*par->name=="\\sum" || *par->name=="\\equals") {
		// sums or equal signs are no problem since the other terms do not end up in our
		// factor; therefore, just go up.
		it=par;
		goto loopie;
		}
	else if(*par->name=="\\fermibilinear" || inh) {
		// For each _other_ child in this product, do a top-down classify for all non-sub/super
      // children; add the indices thus found to the maps since they will end up in our factor.
		Ex::sibling_iterator sit=par.begin();
		while(sit!=par.end()) {
			if(sit!=Ex::sibling_iterator(it)) {
				if(sit->is_index()==false) {
					index_map_t factor_free, factor_dummy;
					classify_indices(sit, factor_free, factor_dummy);
					
					// Test for absence of triple or quadruple indices
					index_map_t must_be_empty;
					determine_intersection(factor_free, ind_dummy, must_be_empty);
					if(must_be_empty.size()>0)
						throw ConsistencyException("Triple index occurred.");
					
					// Test for absence of double index pairs
					must_be_empty.clear();
					determine_intersection(factor_dummy, ind_dummy, must_be_empty);
					if(must_be_empty.size()>0)
						throw ConsistencyException("Double index pair occurred.");
					
					ind_dummy.insert(factor_dummy.begin(), factor_dummy.end());
					index_map_t new_dummy;
					determine_intersection(factor_free, ind_free, new_dummy, true);
					ind_free.insert(factor_free.begin(), factor_free.end());
					ind_dummy.insert(new_dummy.begin(), new_dummy.end());
					}
				else {
//					ind_free.insert(free_so_far.begin(), free_so_far.end());
//					free_so_far.clear();
					classify_add_index(sit, ind_free, ind_dummy);
					}
				}
			++sit;
			}
		it=par;
		goto loopie;
		}
	else if((*par->name).size()>0 && (*par->name)[0]=='@') { // command nodes swallow everything
		return;
		}
	else if(*par->name=="\\tie") { // tie lists do not care about indices
		ind_free.clear();
		ind_dummy.clear();
		it=par;
		goto loopie;
		}
	else if(*par->name=="\\arrow") { // rules can have different indices on lhs and rhs
//		ind_free.clear();
//		ind_dummy.clear();
		it=par;
		goto loopie;
		}
//	else if(*par->name=="\\indexbracket") { // it's really just a bracket, so go up
//		Ex::sibling_iterator sit=tr.begin(par);
//		++sit;
//		while(sit!=tr.end(par)) {
//			++sit;
//			}
//		it=par;
//		goto loopie;
//		} 
	else if(*par->name=="\\comma") { // comma lists can contain anything NO: [a_{mu}, b_{nu}]
		// reaching a comma node is like reaching the top of an expression.
		return;
		}

// FIXME: do something with these warnings!!
//	txtout << "Index classification for this expression failed because of " 
//			 << *par->name << " node, disabling index checking." << std::endl;
//	assert(1==0);
	ind_free.clear();
	ind_dummy.clear();
	}

void IndexClassifier::dumpmap(std::ostream& str, const index_map_t& mp) const
	{
	index_map_t::const_iterator dpr=mp.begin();
	while(dpr!=mp.end()) {
		str << *(dpr->first.begin()->name) << " ";
		++dpr;
		}
	str << std::endl;
	}

// This classifies indices top-down, that is, finds the free indices and all dummy 
// index pairs used in the full subtree below a given node.
void IndexClassifier::classify_indices(Ex::iterator it, index_map_t& ind_free, index_map_t& ind_dummy) const
	{
	const IndexInherit *inh=kernel.properties.get<IndexInherit>(it);
	if(*it->name=="\\sum" || *it->name=="\\equals") {
		index_map_t first_free;
		Ex::sibling_iterator sit=it.begin();
		bool is_first_term=true;
		while(sit!=it.end()) {
			if(*sit->multiplier!=0) { // zeroes are always ok
				index_map_t term_free, term_dummy;
				// print_classify_indices(std::cerr, sit);
				classify_indices(sit, term_free, term_dummy);
				if(!is_first_term) {
					index_map_t::iterator fri=first_free.begin();
					while(fri!=first_free.end()) {
						const Coordinate *cdn=kernel.properties.get_composite<Coordinate>(fri->second, true);
						const Symbol     *smb=kernel.properties.get_composite<Symbol>(fri->second, true);
                  // integer, coordinate or symbol indices always ok
						if(fri->second->is_integer()==false && !cdn && !smb) { 
							if(term_free.count((*fri).first)==0) {
								// std::cerr << (*fri).first << std::endl;
								// std::cerr << "did not find Symbol for " << fri->second << std::endl;
								if(*it->name=="\\sum") 
									throw ConsistencyException("Free indices in different terms in a sum do not match.");
								else
									throw ConsistencyException("Free indices on lhs and rhs do not match.");
								}
							}
						++fri;
						}
					fri=term_free.begin();
					while(fri!=term_free.end()) {
						const Coordinate *cdn=kernel.properties.get_composite<Coordinate>(fri->second, true);
						const Symbol     *smb=kernel.properties.get_composite<Symbol>(fri->second, true); //Symbol::get(kernel.properties, fri->second, true);
                  // integer, coordinate or symbol indices always ok
						if(fri->second->is_integer()==false && !cdn && !smb) { 
							if(first_free.count((*fri).first)==0) {
								if(*it->name=="\\sum") {
									// std::cerr << (*fri).first << " not in first term" << std::endl;
									// std::cerr << Ex(it) << std::endl;
									throw ConsistencyException("Free indices in different terms in a sum do not match.");
									}
								else
									throw ConsistencyException("Free indices on lhs and rhs do not match.");
								}
							}
						++fri;
						}
					}
				else {
					first_free=term_free;
					is_first_term=false;
					}
				
				ind_dummy.insert(term_dummy.begin(), term_dummy.end());
				ind_free.insert(term_free.begin(), term_free.end());
				}
			++sit;
			}
		}
	else if(inh) {
		index_map_t free_so_far;
		Ex::sibling_iterator sit=it.begin();
		while(sit!=it.end()) {
			// std::cerr << "testing" << std::endl;
			if(sit->is_index()==false) {
				index_map_t factor_free, factor_dummy;
				classify_indices(sit, factor_free, factor_dummy);
				
				// Test for absence of triple or quadruple indices
				index_map_t must_be_empty;
				determine_intersection(factor_free, ind_dummy, must_be_empty);
				if(must_be_empty.size()>0) 
					 throw ConsistencyException("Triple index " 
													 + *(must_be_empty.begin()->second->name) 
													 + " inside a single factor found.");
				
				// Test for absence of double index pairs
				must_be_empty.clear();
				determine_intersection(factor_dummy, ind_dummy, must_be_empty);
				if(must_be_empty.size()>0)
					throw ConsistencyException("Double index pair " 
													+ *(must_be_empty.begin()->second->name) 
													+ " inside a single factor found.");
				
				ind_dummy.insert(factor_dummy.begin(), factor_dummy.end());
				index_map_t new_dummy;
//				for(auto& ii: factor_free)
//					std::cerr << "factor_free " << Ex(ii.second) << std::endl;
//				for(auto& ii: free_so_far)
//					std::cerr << "free_so_far " << Ex(ii.second) << std::endl;
				determine_intersection(factor_free, free_so_far, new_dummy, true);
//				for(auto& ii: new_dummy)
//					std::cerr << "new dummy " << Ex(ii.second) << std::endl;
				free_so_far.insert(factor_free.begin(), factor_free.end());
				ind_dummy.insert(new_dummy.begin(), new_dummy.end());
				}
			else {
//				ind_free.insert(free_so_far.begin(), free_so_far.end());
//				free_so_far.clear();
				//std::cerr << "adding index " << Ex(sit) << std::endl;
				classify_add_index(sit, free_so_far, ind_dummy);
				}
			++sit;
//			const Derivative *der=kernel.properties.get<Derivative>(it);
//			if(*it->name=="\\indexbracket" || der) { // the other children are indices themselves
//				ind_free.insert(free_so_far.begin(), free_so_far.end());
//				free_so_far.clear();
//				while(sit!=it.end()) {
//					classify_add_index(sit, ind_free, ind_dummy);
//					++sit;
//					}
//				break;
//				}
			}
		ind_free.insert(free_so_far.begin(), free_so_far.end());
		}
//	else if(tr.is_valid(tr.parent(it))==false) {
//		classify_indices(it.begin(), ind_free, ind_dummy);
//		}
	else if(*it->name=="\\tie") {
		ind_free.clear();
		ind_dummy.clear();
		}
	else if(*it->name=="\\pow") {
		// Power nodes can have dummies in all arguments, but no free indices. We allow for
		// \pow{ A_{m} A^{m} }{2} type of things, in the understanding that any algorithm that
		// does something with this (e.g. product_rule) will need to relabel once the expression
		// gets down to A_{m} A^{m} itself. Note that the classifier will mark numerical indices
		// and coordinate indices as free.
		auto sib=it.begin();
		while(sib!=it.end()) {
			index_map_t ind_free_here, ind_dummy_here;
			classify_indices(sib, ind_free_here, ind_dummy_here);
			if(ind_free_here.size()>0) {
				for(auto& di: ind_free_here) {
					const Coordinate *cdn=kernel.properties.get<Coordinate>(di.second, true);
					const Symbol     *smb=kernel.properties.get<Symbol>(di.second, true);
					if(! (cdn || smb || di.second->is_integer()) ) {
#ifdef DEBUG
						std::cerr << di.first << std::endl;
#endif
						throw ConsistencyException("Power with free indices not allowed.");
						}
					}
//				ind_free_here.clear();
				}
			// FIXME: add test for overlap
			ind_free.insert(ind_free_here.begin(), ind_free_here.end());
			ind_dummy.insert(ind_dummy_here.begin(), ind_dummy_here.end());
			++sib;
			}
		}
	else if((*it->name).size()>0 && (*it->name)[0]=='@') {
		// This is an active node that has not been replaced yet; since
		// we do not know anything about what this will become, do not return
		// any index information (clashes will be resolved when the active
		// node gets replaced).
		}
	else {
		Ex::sibling_iterator sit=it.begin();
		index_map_t item_free;
		index_map_t item_dummy;
		while(sit!=it.end()) {
//			std::cout << *sit->name << std::endl;
			if((sit->fl.parent_rel==str_node::p_sub || sit->fl.parent_rel==str_node::p_super) && sit->fl.bracket==str_node::b_none) {
				if(*sit->name!="??") {
					const Coordinate *cdn=kernel.properties.get<Coordinate>(sit, true);
					const Symbol     *smb=kernel.properties.get<Symbol>(sit, true);
					// integer, coordinate or symbol indices always ok
					if(sit->is_integer() || cdn || smb) {
						// Note: even integers need to be stored as indices, because we expect e.g. canonicalise
						// to re-order even numerical indices. They should just never be flagged as dummies.
						item_free.insert(index_map_t::value_type(Ex(sit), Ex::iterator(sit)));
						}
					else {
						index_map_t::iterator fnd=find_modulo_parent_rel(sit, item_free);
//						index_map_t::iterator fnd=item_free.find(Ex(sit));
						if(fnd!=item_free.end()) {
//							std::cerr << *sit->name << " already in free set" << std::endl;
//							if(item_dummy.find(Ex(sit))!=item_dummy.end())
							if(find_modulo_parent_rel(sit, item_dummy)!=item_dummy.end())
								throw ConsistencyException("Triple index " + *sit->name + " inside a single factor found.");
							item_dummy.insert(*fnd);
							item_free.erase(fnd);
							item_dummy.insert(index_map_t::value_type(Ex(sit), Ex::iterator(sit)));
//							std::cout << item_dummy.size() << " " << item_free.size() << std::endl;
							}
						else {
//							std::cerr << *sit->name << " is new" << std::endl;
							item_free.insert(index_map_t::value_type(Ex(sit), Ex::iterator(sit)));
							}
						}
					}
				}
//			else {
//				item_free.insert(index_map_t::value_type(sit->name, Ex::iterator(sit)));
//				}
			++sit;
			}
		ind_free.insert(item_free.begin(), item_free.end());
		ind_dummy.insert(item_dummy.begin(), item_dummy.end());
		}
//	std::cout << "ind_free: " << ind_free.size() << std::endl;
//	std::cout << "ind_dummy: " << ind_dummy.size() << std::endl;

	}

Ex IndexClassifier::get_dummy(const list_property *dums,
												  const index_map_t * one, 
												  const index_map_t * two,
												  const index_map_t * three,
												  const index_map_t * four,
												  const index_map_t * five) const
	{
	std::pair<Properties::pattern_map_t::const_iterator, Properties::pattern_map_t::const_iterator>
		pr=kernel.properties.pats.equal_range(dums);

	// std::cerr << "finding index not in: " << std::endl;
	// if(one)
	// 	for(auto& i: *one)
	// 		std::cerr << i.first << std::endl;
	// if(two)
	// 	for(auto& i: *two)
	// 		std::cerr << i.first << std::endl;
	// if(three)
	// 	for(auto& i: *three)
	// 		std::cerr << i.first << std::endl;
	// if(four)
	// 	for(auto& i: *four)
	// 		std::cerr << i.first << std::endl;
	// if(five)
	// 	for(auto& i: *five)
	// 		std::cerr << i.first << std::endl;
	
	while(pr.first!=pr.second) {
		// std::cerr << "trying " << pr.first->second->obj << std::endl;
		if(pr.first->second->obj.begin()->is_autodeclare_wildcard()) {
			std::string base=*pr.first->second->obj.begin()->name_only();
			int used=max_numbered_name(base, one, two, three, four, five);
			std::ostringstream str;
			str << base << used+1;
//			txtout << "going to use " << str.str() << std::endl;
			nset_t::iterator newnm=name_set.insert(str.str()).first;
			Ex ret;
			ret.set_head(str_node(newnm));
			return ret;
			}
		else {
			const Ex& inm=(*pr.first).second->obj;
			// BUG: even if only _{a} is in the used map, we should not
			// accept ^{a}. But since ...
			if(index_in_set(inm, one)==false   &&
				index_in_set(inm, two)==false   &&
				index_in_set(inm, three)==false &&
				index_in_set(inm, four)==false  &&
				index_in_set(inm, five)==false) {
				// std::cerr << "ok to use " << inm << std::endl;
				return inm;
				}
			}
		++pr.first;
		}

	const Indices *dd=dynamic_cast<const Indices *>(dums);
	assert(dd);
	throw ConsistencyException("Ran out of dummy indices for type \""+dd->set_name+"\".");
	}

Ex IndexClassifier::get_dummy(const list_property *dums, Ex::iterator it) const
	{
	index_map_t one, two, three, four, five;
	classify_indices_up(it, one, two);
	classify_indices(it, three, four);
	
	return get_dummy(dums, &one, &two, &three, &four, 0);
	}

// Find a dummy index of the type given in "nm", making sure that this index
// name does not class with the object in it1 nor it2.

Ex IndexClassifier::get_dummy(const list_property *dums, Ex::iterator it1, Ex::iterator it2) const
	{
	index_map_t one, two, three, four, five;
	classify_indices_up(it1, one, two);
	classify_indices_up(it2, one, two);
	classify_indices(it1, three, four);
	classify_indices(it2, three, four);
	
	return get_dummy(dums, &one, &two, &three, &four, 0);
	}

void IndexClassifier::print_classify_indices(std::ostream& str, Ex::iterator st) const
	{
	str << "for node " << Ex(st) << std::endl;
	index_map_t ind_free, ind_dummy;
	classify_indices(st, ind_free, ind_dummy);
	
	index_map_t::iterator it=ind_free.begin();
	index_map_t::iterator prev=ind_free.end();
	str << "free indices: " << std::endl;
	while(it!=ind_free.end()) {
		if(prev==ind_free.end() || tree_exact_equal(&kernel.properties, (*it).first,(*prev).first,1,true,-2,true)==false)
			str << *(*it).first.begin()->name << " (" << ind_free.count((*it).first) << ") ";
		prev=it;
		++it;
		}
	str << std::endl;
	it=ind_dummy.begin();
	prev=ind_dummy.end();
	str << "dummy indices: ";
	while(it!=ind_dummy.end()) {
		if(prev==ind_dummy.end() || tree_exact_equal(&kernel.properties, (*it).first,(*prev).first,1,true,-2,true)==false)
			str << *(*it).first.begin()->name << " (" << ind_dummy.count((*it).first) << ") ";
		prev=it;
		++it;
		}
	str << "---" << std::endl;
	}

int IndexClassifier::max_numbered_name_one(const std::string& nm, const index_map_t * one) const
	{
	assert(one);

	int themax=0;
	index_map_t::const_iterator it=one->begin();
	while(it!=one->end()) {
		size_t pos=(*it->first.begin()->name).find_first_of("0123456789");
		if(pos!=std::string::npos) {
//			txtout << (*it->first).substr(0,pos) << std::endl;
			if((*it->first.begin()->name).substr(0,pos) == nm) {
				int thenum=atoi((*it->first.begin()->name).substr(pos).c_str());
//				txtout << "num = " << thenum << std::endl;
				themax=std::max(themax, thenum);
				}
			}
		++it;
		}
	return themax;
	}

int IndexClassifier::max_numbered_name(const std::string& nm, 
											const index_map_t * one, 
											const index_map_t * two,
											const index_map_t * three,
											const index_map_t * four,
											const index_map_t * five) const
	{
	int themax=0;
	if(one) {
		themax=std::max(themax, max_numbered_name_one(nm, one));
		if(two) {
			themax=std::max(themax, max_numbered_name_one(nm, two));
			if(three) {
				themax=std::max(themax, max_numbered_name_one(nm, three));
				if(four) {
					themax=std::max(themax, max_numbered_name_one(nm, four));
					if(five) {
						themax=std::max(themax, max_numbered_name_one(nm, five));
						}
					}
				}
			}
		}
	return themax;
	}


bool IndexClassifier::index_in_set(Ex ex, const index_map_t *im) const
	{
	if(im==0) return false;

	if(im->count(ex)>0) return true;

	if(ex.begin()->fl.parent_rel==str_node::p_super) {
		ex.begin()->fl.parent_rel=str_node::p_sub;
		int c=im->count(ex);
		if(c>0) return true;
		}
	if(ex.begin()->fl.parent_rel==str_node::p_sub) {
		ex.begin()->fl.parent_rel=str_node::p_super;
		int c=im->count(ex);
		if(c>0) return true;
		}
	return false;
	}
