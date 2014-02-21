
#include "Compare.hh"

int subtree_compare(exptree::iterator one, exptree::iterator two, 
						  int mod_prel, bool checksets, int compare_multiplier, bool literal_wildcards) 
	{
	// The logic is to compare successive aspects of the two objects, returning a
	// no-match code if a difference is found at a particular level, or continuing
	// further down the line if there still is a match.
	
	// Compare multipliers. Skip this step if one of the objects is a rational and the
	// other not, as in that case we are matching values to symbols.
	if( one->is_rational()==two->is_rational() ) {
		if(compare_multiplier==-2 && !two->is_name_wildcard() && !one->is_name_wildcard())
			if(one->multiplier != two->multiplier) {
				if(*one->multiplier < *two->multiplier) return 2;
				else return -2;
				}
		}

	// First lookup some information about the index sets, if any.
	// (note: to avoid having properties::get enter here recursively, we
	// perform this check only when both objects are sub/superscripts, i.e. is_index()==true).
	// If one and two are sub/superscript, and sit in the same Indices, we keep mult=1, all
	// other cases get mult=2. 

	int  mult=1;
	if(one->is_index() && two->is_index() && one->is_rational() && two->is_rational()) mult=2;
	Indices::position_t position_type=Indices::free;
	if(one->is_index() && two->is_index()) {
		if(checksets) {
			// Strip off the parent_rel because Indices properties are declared without
			// those.
			const Indices *ind1=kernel.properties.get<Indices>(one, true);
			const Indices *ind2=kernel.properties.get<Indices>(two, true);
			if(ind1!=ind2) { 
				// It may still be that one set is a subset of the other, i.e that the
				// parent argument of Indices has been used.
				mult=2;
				// FIXME: this is required for implicit symmetry patterns on split_index objects
				//			if(ind1!=0 && ind2!=0) 
				//				if(ind1->parent_name==ind2->set_name || ind2->parent_name==ind1->set_name)
				//					mult=1;
				}
			if(ind1!=0 && ind1==ind2) 
				position_type=ind1->position_type;
			}
		}
	else mult=2;
	
	// Compare sub/superscript relations.
	if((mod_prel==-2 && position_type!=Indices::free) && one->is_index() && two->is_index() ) {
		if(one->fl.parent_rel!=two->fl.parent_rel) {
			if(one->fl.parent_rel==str_node::p_sub) return 2;
			else return -2;
			}
		}

	// Handle object wildcards and comparison
	if(!literal_wildcards) {
		if(one->is_object_wildcard() || two->is_object_wildcard())
			return 0;
		}

	// Handle mismatching node names.
	if(one->name!=two->name) {
		if(literal_wildcards) {
			if(*one->name < *two->name) return mult;
			else return -mult;
			}

		if( (one->is_autodeclare_wildcard() && two->is_numbered_symbol()) || (two->is_autodeclare_wildcard() && one->is_numbered_symbol()) ) {
			if( one->name_only() != two->name_only() ) {
				if(*one->name < *two->name) return mult;
				else return -mult;
				}
			}
		else if( one->is_name_wildcard()==false && two->is_name_wildcard()==false ) {
			if(*one->name < *two->name) return mult;
			else return -mult;
			}
		}

	// Now turn to the child nodes. Before comparing them directly, first compare
	// the number of children, taking into account range wildcards.
	int numch1=exptree::number_of_children(one);
	int numch2=exptree::number_of_children(two);

//	if(numch1>0 && one.begin()->is_range_wildcard()) {
		// FIXME: insert the code from props.cc here, ditto in the next if.
//		return 0;
//		}

//	if(numch2>0 && two.begin()->is_range_wildcard()) return 0;

	if(numch1!=numch2) {
		if(numch1<numch2) return 2;
		else return -2;
		}

	// Compare actual children.
	exptree::sibling_iterator sib1=one.begin(), sib2=two.begin();
	int remember_ret=0;
	if(mod_prel==0) mod_prel=-2;
	else if(mod_prel>0)  --mod_prel;
	if(compare_multiplier==0) compare_multiplier=-2;
	else if(compare_multiplier>0)  --compare_multiplier;

	while(sib1!=one.end()) {
		int ret=subtree_compare(sib1,sib2, mod_prel, checksets, compare_multiplier, literal_wildcards);
		if(abs(ret)>1)
			return ret/abs(ret)*mult;
		if(ret!=0 && remember_ret==0) 
			remember_ret=ret;
		++sib1;
		++sib2;
		}
	return remember_ret;
	}

bool tree_less(const exptree& one, const exptree& two, int mod_prel, bool checksets, int compare_multiplier)
	{
	return subtree_less(one.begin(), two.begin(), mod_prel, checksets, compare_multiplier);
	}

bool tree_equal(const exptree& one, const exptree& two, int mod_prel, bool checksets, int compare_multiplier)
	{
	return subtree_equal(one.begin(), two.begin(), mod_prel, checksets, compare_multiplier);
	}

bool tree_exact_less(const exptree& one, const exptree& two, int mod_prel, bool checksets, int compare_multiplier, bool literal_wildcards)
	{
	return subtree_exact_less(one.begin(), two.begin(), mod_prel, checksets, compare_multiplier, literal_wildcards);
	}

bool tree_exact_equal(const exptree& one, const exptree& two, int mod_prel, bool checksets, int compare_multiplier, bool literal_wildcards)
	{
	return subtree_exact_equal(one.begin(), two.begin(), mod_prel, checksets, compare_multiplier, literal_wildcards);
	}

bool subtree_less(exptree::iterator one, exptree::iterator two, int mod_prel, bool checksets, int compare_multiplier)
	{
	int cmp=subtree_compare(one, two, mod_prel, checksets, compare_multiplier);
	if(cmp==2) return true;
	return false;
	}

bool subtree_equal(exptree::iterator one, exptree::iterator two, int mod_prel, bool checksets, int compare_multiplier)
	{
	int cmp=subtree_compare(one, two, mod_prel, checksets, compare_multiplier);
	if(abs(cmp)<=1) return true;
	return false;
	}

bool subtree_exact_less(exptree::iterator one, exptree::iterator two, int mod_prel, bool checksets, int compare_multiplier, bool literal_wildcards)
	{
	int cmp=subtree_compare(one, two, mod_prel, checksets, compare_multiplier, literal_wildcards);
	if(cmp>0) return true;
	return false;
	}

bool subtree_exact_equal(exptree::iterator one, exptree::iterator two, int mod_prel, bool checksets, int compare_multiplier, bool literal_wildcards)
	{
	int cmp=subtree_compare(one, two, mod_prel, checksets, compare_multiplier, literal_wildcards);
	if(cmp==0) return true;
	return false;
	}

bool tree_less_obj::operator()(const exptree& one, const exptree& two) const
	{
	return tree_less(one, two);
	}

bool tree_less_modprel_obj::operator()(const exptree& one, const exptree& two) const
	{
	return tree_less(one, two, 0);
	}

bool tree_equal_obj::operator()(const exptree& one, const exptree& two) const
	{
	return tree_equal(one, two);
	}

bool tree_exact_less_obj::operator()(const exptree& one, const exptree& two) const
	{
	return tree_exact_less(one, two);
	}

bool tree_exact_less_no_wildcards_obj::operator()(const exptree& one, const exptree& two) const
	{
	return tree_exact_less(one, two, -2, true, 0, true);
	}

bool tree_exact_less_no_wildcards_mod_prel_obj::operator()(const exptree& one, const exptree& two) const
	{
	return tree_exact_less(one, two, 0, true, -2, true);
	}

bool tree_exact_equal_obj::operator()(const exptree& one, const exptree& two) const
	{
	return tree_exact_equal(one, two);
	}

bool tree_exact_less_mod_prel_obj::operator()(const exptree& one, const exptree& two) const
	{
	return tree_exact_less(one, two, 0, true, -2, true);
	}

bool tree_exact_equal_mod_prel_obj::operator()(const exptree& one, const exptree& two) const
	{
	return tree_exact_equal(one, two, 0, true, -2, true);
	}

bool operator==(const exptree& first, const exptree& second)
	{
	return tree_exact_equal(first, second, 0, true, -2, true);
	}


void exptree_comparator::clear()
	{
	replacement_map.clear();
	subtree_replacement_map.clear();
	factor_locations.clear();
	factor_moving_signs.clear();
	}

exptree_comparator::match_t exptree_comparator::equal_subtree(exptree::iterator i1, exptree::iterator i2)
	{
	exptree::sibling_iterator i1end(i1);
	exptree::sibling_iterator i2end(i2);
	++i1end;
	++i2end;

	bool first_call=true;
	while(i1!=i1end && i2!=i2end) {
		match_t mm=compare(i1,i2,first_call);
		first_call=false;
		switch(mm) {
			case no_match_less:
			case no_match_greater:
				return mm;
			case node_match: {
				size_t num1=exptree::number_of_children(i1);
				size_t num2=exptree::number_of_children(i2);
				if(num1 < num2)      return no_match_less;
				else if(num1 > num2) return no_match_greater;
				break;
				}
			case subtree_match:
				i1.skip_children();
				i2.skip_children();
				break;
			}
		++i1;
		++i2;
		}

	return subtree_match;
	}

exptree_comparator::match_t exptree_comparator::compare(const exptree::iterator& one, 
																		  const exptree::iterator& two, 
																		  bool nobrackets) 
	{
	// nobrackets also implies 'no multiplier', i.e. 'toplevel'.
	// one is the substitute pattern, two the expression under consideration
	
//	std::cerr << "matching " << *one->name << " to " << *two->name << std::endl;

	if(nobrackets==false && one->fl.bracket != two->fl.bracket) 
		return (one->fl.bracket < two->fl.bracket)?no_match_less:no_match_greater;

//	std::cerr << "one passed" << std::endl;

	// FIXME: this needs to be relaxed for position-free indices
	if(one->fl.parent_rel != two->fl.parent_rel)                
		return (one->fl.parent_rel < two->fl.parent_rel)?no_match_less:no_match_greater;

//	std::cerr << "two passed" << std::endl;

	// Determine whether we are dealing with one of the pattern types.
	bool pattern=false;
	bool objectpattern=false;
	bool implicit_pattern=false;
	bool is_index=false;
	
	if(one->fl.bracket==str_node::b_none && one->is_index() ) 
		is_index=true;
	if(one->is_name_wildcard())
		pattern=true;
	else if(one->is_object_wildcard())
		objectpattern=true;
	else if(is_index && one->is_integer()==false) {
		const Coordinate *cdn1=kernel.properties.get<Coordinate>(one, true);
		if(cdn1==0)
			implicit_pattern=true;
		}
		
	if(pattern || (implicit_pattern && two->is_integer()==false)) { 
		// The above is to ensure that we never match integers to implicit patterns.

		// We want to search the replacement map for replacement rules which we have
		// constructed earlier, and discard the current match if it conflicts those 
		// rules. This is to make sure that e.g. a pattern k1_a k2_a does not match 
		// an expression k1_c k2_d.
		// 
		// In order to ensure that a replacement rule for a lower index is also
		// triggering a rule for an upper index, we simply store both rules (see
		// below) so that searching for rules can remain simple.

		replacement_map_t::iterator loc=replacement_map.find(one);

		bool tested_full=true;

		// If this is a pattern with a non-zero number of children, 
		// also search the pattern without the children.
		if(loc == replacement_map.end() && exptree::number_of_children(one)!=0) {
			exptree tmp1(one);
			tmp1.erase_children(tmp1.begin());
			loc = replacement_map.find(tmp1);
			tested_full=false;
			}

		if(loc!=replacement_map.end()) {
//			std::cerr << "found!" << std::endl;
			// If this is an index/pattern, try to match the whole index/pattern.
			int cmp;

			if(tested_full) 
				cmp=subtree_compare((*loc).second.begin(), two, -2 /* KP: do not switch this to -2 (kk.cdb fails) */); 
			else {
				exptree tmp2(two);
				tmp2.erase_children(tmp2.begin());
				cmp=subtree_compare((*loc).second.begin(), tmp2.begin(), -2 /* KP: see above */); 
				}
//			std::cerr << " pattern " << *two->name
//						 << " should be " << *((*loc).second.begin()->name)  
//						 << " because that's what " << *one->name 
//						 << " was set to previously; result " << cmp << std::endl;

			if(cmp==0)      return subtree_match;
			else if(cmp>0)  return no_match_less;
			else            return no_match_greater;
			}
		else {
			// This index/pattern was not encountered earlier. Check that the index types in pattern
			// and object agree (if known, otherwise assume they match)

//			std::cerr << "index check " << *one->name << " " << *two->name << std::endl;

			const Indices *t1=kernel.properties.get<Indices>(one, true);
			const Indices *t2=kernel.properties.get<Indices>(two, true);
//			std::cerr << t1 << " " << t2 << std::endl;
			if( (t1 || t2) && implicit_pattern ) {
				if(t1 && t2) {
					if((*t1).set_name != (*t2).set_name) {
						if((*t1).set_name < (*t2).set_name) return no_match_less;
						else                                return no_match_greater;
						}
					}
				else {
					if(t1) return no_match_less;
					else   return no_match_greater;
					}
				}
			// The index types match, so register this replacement rule.
//			std::cerr << "registering ";
//			if(one->fl.parent_rel==str_node::p_super) std::cerr << "^";
//			if(one->fl.parent_rel==str_node::p_sub)   std::cerr << "_";
//			std::cerr << *one->name << " ";
//			if(two->fl.parent_rel==str_node::p_super) std::cerr << "^";
//			if(two->fl.parent_rel==str_node::p_sub)   std::cerr << "_";
//			std::cerr << *two->name << std::endl;

			replacement_map[one]=two;
			
			// if this is an index, also store the pattern with the parent_rel flipped
			if(one->is_index()) {
				exptree cmptree1(one);
				exptree cmptree2(two);
				cmptree1.begin()->flip_parent_rel();
				if(two->is_index())
					cmptree2.begin()->flip_parent_rel();
				replacement_map[cmptree1]=cmptree2;
				}
			
			// if this is a pattern and the pattern has a non-zero number of children,
			// also add the pattern without the children
			if(exptree::number_of_children(one)!=0) {
				exptree tmp1(one), tmp2(two);
				tmp1.erase_children(tmp1.begin());
				tmp2.erase_children(tmp2.begin());
				replacement_map[tmp1]=tmp2;
				}
			// and if this is a pattern also insert the one without the parent_rel
			if(one->is_name_wildcard()) {
				exptree tmp1(one), tmp2(two);
				tmp1.begin()->fl.parent_rel=str_node::p_none;
				tmp2.begin()->fl.parent_rel=str_node::p_none;
				replacement_map[tmp1]=tmp2;
				}
			}
		
		// Return a match of the appropriate type
		if(is_index) return subtree_match;
		else         return node_match;
		}
	else if(objectpattern) {
		subtree_replacement_map_t::iterator loc=subtree_replacement_map.find(one->name);
		if(loc!=subtree_replacement_map.end()) {
			return equal_subtree((*loc).second,two);
			}
		else subtree_replacement_map[one->name]=two;
		
		return subtree_match;
		}
	else { // object is not dummy
		if(one->is_rational() && two->is_rational() && one->multiplier!=two->multiplier) {
			if(*one->multiplier < *two->multiplier) return no_match_less;
			else                                    return no_match_greater;
			}
		
		if(one->name==two->name) {
			if(nobrackets || (one->multiplier == two->multiplier) ) 
				return node_match;

			if(*one->multiplier < *two->multiplier) return no_match_less;
			else                                    return no_match_greater;
			}
		else {
			if( *one->name < *two->name ) return no_match_less;
			else                          return no_match_greater;
			}
		}
	
	assert(1==0); // should never be reached

	return no_match_less; 
	}


// Find a subproduct in a product. The 'lhs' iterator points to the product which
// we want to find, the 'tofind' iterator to the current factor which we are looking
// for. The product in which to search is pointed to by 'st'.
//
// Once 'tofind' is found, this routine calls itself to find the next factor in
// 'lhs'. If the next factor cannot be found, we backtrack and try to find the
// previous factor again (it may have appeared multiple times).
//
exptree_comparator::match_t exptree_comparator::match_subproduct(exptree::sibling_iterator lhs, 
																					  exptree::sibling_iterator tofind, 
																					  exptree::sibling_iterator st)
	{
	replacement_map_t         backup_replacements(replacement_map);
	subtree_replacement_map_t backup_subtree_replacements(subtree_replacement_map);

	exptree::sibling_iterator start=st.begin();
	while(start!=st.end()) {
		if(std::find(factor_locations.begin(), factor_locations.end(), start)==factor_locations.end()) {  
			if(equal_subtree(tofind, start)==subtree_match) { // found factor
				// If a previous factor was found, verify that the factor found now can be
				// moved next to the previous factor (nontrivial if factors do not commute).
				int sign=1;
				if(factor_locations.size()>0) {
					sign=exptree_ordering::can_move_adjacent(st, factor_locations.back(), start);
					}
				if(sign==0) { // object found, but we cannot move it in the right order
					replacement_map=backup_replacements;
					subtree_replacement_map=backup_subtree_replacements;
					}
				else {
					factor_locations.push_back(start);
					factor_moving_signs.push_back(sign);
					
					exptree::sibling_iterator nxt=tofind; 
					++nxt;
					if(nxt!=lhs.end()) {
						match_t res=match_subproduct(lhs, nxt, st);
						if(res==subtree_match) return res;
						else {
//						txtout << tofind.node << "found factor useless " << start.node << std::endl;
							factor_locations.pop_back();
							factor_moving_signs.pop_back();
							replacement_map=backup_replacements;
							subtree_replacement_map=backup_subtree_replacements;
							}
						}
					else return subtree_match;
					}
				}
			else {
//				txtout << tofind.node << "does not match" << std::endl;
				replacement_map=backup_replacements;
				subtree_replacement_map=backup_subtree_replacements;
				}
			}
		++start;
		}
	return no_match_less; // FIXME not entirely true
	}


// Determine whether the two objects can be moved next to each other,
// with 'one' to the left of 'two'. Return the sign, or zero.
//
int exptree_ordering::can_move_adjacent(exptree::iterator prod,
													 exptree::sibling_iterator one, exptree::sibling_iterator two) 
	{
	assert(exptree::parent(one)==exptree::parent(two));
	assert(exptree::parent(one)==prod);

	// Make sure that 'one' points to the object which occurs first in 'prod'.
	bool onefirst=false;
	exptree::sibling_iterator probe=one;
	while(probe!=prod.end()) {
		if(probe==two) {
			onefirst=true;
			break;
			}
		++probe;
		}
	int sign=1;
	if(!onefirst) {
		std::swap(one,two);
		int es=subtree_compare(one,two);
		sign*=can_swap(one,two,es);
//		txtout << "swapping one and two: " << sign << std::endl;
		}

	if(sign!=0) {
		// Loop over all pair flips which are necessary to move one to the left of two.
		probe=one;
		++probe;
		while(probe!=two) {
			assert(probe!=prod.end());
			int es=subtree_compare(one,probe);
			sign*=can_swap(one,probe,es);
			if(sign==0) break;
			++probe;
			}
		}
	return sign;
	}



// Should obj and obj+1 be swapped, according to the SortOrder
// properties?
//
bool exptree_ordering::should_swap(exptree::iterator obj, int subtree_comparison) 
	{
	exptree::sibling_iterator one=obj, two=obj;
	++two;

	// Find a SortOrder property which contains both one and two.
	int num1, num2;
	const SortOrder *so1=kernel.properties.get_composite<SortOrder>(one,num1);
	const SortOrder *so2=kernel.properties.get_composite<SortOrder>(two,num2);

//	std::cerr << so1 << " " << so2 << " " << subtree_comparison << std::endl;

	if(so1==0 || so2==0) { // No sort order known
		if(subtree_comparison<0) return true;
		return false;
		}
	else if(abs(subtree_comparison)<=1) { // Identical up to index names
		if(subtree_comparison==-1) return true;
		return false;
		}
	else {
//		std::cerr << num1 << " " << num2 << std::endl;
		if(so1==so2) {
			if(num1>num2) return true;
			return false;
			}
		}

	return false;
	}

// Various tests about whether two non-elementary objects can be swapped.
//
int exptree_ordering::can_swap_prod_obj(exptree::iterator prod, exptree::iterator obj, 
													 bool ignore_implicit_indices) 
	{
//	std::cout << "prod_obj " << *prod->name << " " << *obj->name << std::endl;
	// Warning: no check is made that prod is actually a product!
	int sign=1;
	exptree::sibling_iterator sib=prod.begin();
	while(sib!=prod.end()) {
		const Indices *ind1=kernel.properties.get_composite<Indices>(sib, true);
		const Indices *ind2=kernel.properties.get_composite<Indices>(obj, true);
		if(! (ind1!=0 && ind2!=0) ) { // If both objects are actually real indices, 
			                           // then we do not include their commutativity property
			                           // in the sign. This is because the routines that use
                                    // can_swap_prod_obj all test for such index-index 
                                    // swaps separately.
			int es=subtree_compare(sib, obj, 0);
//			std::cout << "  " << *sib->name << " " << *obj->name << " " << es << std::endl;
			sign*=can_swap(sib, obj, es, ignore_implicit_indices);
			if(sign==0) break;
			}
		++sib;
		}
	return sign;
	}

int exptree_ordering::can_swap_prod_prod(exptree::iterator prod1, exptree::iterator prod2, 
													 bool ignore_implicit_indices)  
	{
//	std::cout << "prod_prod " << *prod1->name << " " << *prod2->name;
	// Warning: no check is made that prod1,2 are actually products!
	int sign=1;
	exptree::sibling_iterator sib=prod2.begin();
	while(sib!=prod2.end()) {
		sign*=can_swap_prod_obj(prod1, sib, ignore_implicit_indices);
		if(sign==0) break;
		++sib;
		}
//	std::cout << "  -> " << sign << std::endl;
	return sign;
	}

int exptree_ordering::can_swap_sum_obj(exptree::iterator sum, exptree::iterator obj, 
													bool ignore_implicit_indices) 
	{
	// Warning: no check is made that sum is actually a sum!
	int sofar=2;
	exptree::sibling_iterator sib=sum.begin();
	while(sib!=sum.end()) {
		int es=subtree_compare(sib, obj);
		int thissign=can_swap(sib, obj, es, ignore_implicit_indices);
		if(sofar==2) sofar=thissign;
		else if(thissign!=sofar) {
			sofar=0;
			break;
			}
		++sib;
		}
	return sofar;
	}

int exptree_ordering::can_swap_prod_sum(exptree::iterator prod, exptree::iterator sum, 
													 bool ignore_implicit_indices) 
	{
	// Warning: no check is made that sum is actually a sum or prod is a prod!
	int sign=1;
	exptree::sibling_iterator sib=prod.begin();
	while(sib!=prod.end()) {
//		const Indices *ind=kernel.properties.get_composite<Indices>(sib);
//		if(ind==0) {
		sign*=can_swap_sum_obj(sum, sib, ignore_implicit_indices);
			if(sign==0) break;
//			}
		++sib;
		}
	return sign;
	}

int exptree_ordering::can_swap_sum_sum(exptree::iterator sum1, exptree::iterator sum2,
													bool ignore_implicit_indices) 
	{
	int sofar=2;
	exptree::sibling_iterator sib=sum1.begin();
	while(sib!=sum1.end()) {
		int thissign=can_swap_sum_obj(sum2, sib, ignore_implicit_indices);
		if(sofar==2) sofar=thissign;
		else if(thissign!=sofar) {
			sofar=0;
			break;
			}
		++sib;
		}
	return sofar;
	}

int exptree_ordering::can_swap_ilist_ilist(const Kernel& kernel, exptree::iterator obj1, exptree::iterator obj2) 
	{
	int sign=1;

	exptree::index_iterator it1=exptree::begin_index(kernel, obj1);
	while(it1!=exptree::end_index(obj1)) {
		exptree::index_iterator it2=exptree::begin_index(kernel, obj2);
		while(it2!=exptree::end_index(obj2)) {
			// Only deal with real indices here, i.e. those carrying an Indices property.
			const Indices *ind1=kernel.properties.get_composite<Indices>(it1, true);
			const Indices *ind2=kernel.properties.get_composite<Indices>(it2, true);
			if(ind1!=0 && ind2!=0) {
				const CommutingBehaviour *com1 =kernel.properties.get_composite<CommutingBehaviour>(it1, true);
				const CommutingBehaviour *com2 =kernel.properties.get_composite<CommutingBehaviour>(it2, true);
				
				if(com1!=0  &&  com1 == com2) 
					sign *= com1->sign();
				
				if(sign==0) break;
				}
			++it2;
			}
		if(sign==0) break;
		++it1;
		}

	return sign;
	}

// Can obj and obj+1 be exchanged? If yes, return the sign,
// if no return zero. This is the general entry point for 
// two arbitrary nodes (which may be a product or sum). 
// Do not call the functions above directly!
//
// The last flag ('ignore_implicit_indices') is used to disable 
// all checks dealing with implicit indices (this is useful for
// algorithms which re-order objects with implicit indices, which would
// otherwise always receive a 0 from this function).
//
int exptree_ordering::can_swap(exptree::iterator one, exptree::iterator two, int subtree_comparison,
										 bool ignore_implicit_indices) 
	{
//	std::cout << "can_swap " << *one->name << " " << *two->name << ignore_implicit_indices << std::endl;

	const ImplicitIndex *ii1 = kernel.properties.get_composite<ImplicitIndex>(one);
	const ImplicitIndex *ii2 = kernel.properties.get_composite<ImplicitIndex>(two);

	// When both objects carry an implicit index but the index lines are not connected,
	// we should not be using explicit commutation rules, as this would mess up the
	// index lines and make the expression meaningless.
	// FIXME: this would ideally make use of index and conjugate index lines.

//	const DiracBar *db2 = kernel.properties.get_composite<DiracBar>(two);
	if(! (ii1 && ii2 /* && db2 */) ) {

		// First of all, check whether there is an explicit declaration for the commutativity 
		// of these two symbols.
//		std::cout << *one->name << " explicit " << *two->name << std::endl;
		const CommutingBehaviour *com = kernel.properties.get_composite<CommutingBehaviour>(one, two, true);
		
		if(com) {
//			std::cout << "explicit " << com->sign() << std::endl;
			return com->sign();
			}
		}
	
	if(ignore_implicit_indices==false) {
		// Two implicit-index objects cannot move through each other if they have the
		// same type of implicit index.
//		std::cout << "can_swap " << *one->name << " " << *two->name << std::endl;

		if(ii1 && ii2) {
			if(ii1->set_names.size()==0 && ii2->set_names.size()==0) return 0; // empty index name
			for(size_t n1=0; n1<ii1->set_names.size(); ++n1)
				for(size_t n2=0; n2<ii2->set_names.size(); ++n2)
					if(ii1->set_names[n1]==ii2->set_names[n2])
						return 0;
			}
		}

	// Do we need to use Self* properties?
	const SelfCommutingBehaviour *sc1 =kernel.properties.get_composite<SelfCommutingBehaviour>(one, true);
	const SelfCommutingBehaviour *sc2 =kernel.properties.get_composite<SelfCommutingBehaviour>(two, true);
	if( (sc1!=0 && sc1==sc2) ) 
		return sc1->sign();

	// One or both of the objects are not in an explicit list. So now comes the generic
	// part. The first step is to look at all explicit indices of the two objects and determine 
	// their commutativity. 
	// Note: this does not yet look at arguments (non-index children).

	int tmpsign=can_swap_ilist_ilist(one, two);
	if(tmpsign==0) return 0;
	
	// The second step is to check for product-like and sum-like behaviour. The following
	// take into account all commutativity properties of explict with implicit indices,
	// as well as hard-specified commutativity of factors.

	const CommutingAsProduct *comap1 = kernel.properties.get_composite<CommutingAsProduct>(one);
	const CommutingAsProduct *comap2 = kernel.properties.get_composite<CommutingAsProduct>(two);
	const CommutingAsSum     *comas1 = kernel.properties.get_composite<CommutingAsSum>(one);
	const CommutingAsSum     *comas2 = kernel.properties.get_composite<CommutingAsSum>(two);
	
	if(comap1 && comap2) return tmpsign*can_swap_prod_prod(one,two,ignore_implicit_indices);
	if(comap1 && comas2) return tmpsign*can_swap_prod_sum(one,two,ignore_implicit_indices);
	if(comap2 && comas1) return tmpsign*can_swap_prod_sum(two,one,ignore_implicit_indices);
	if(comas1 && comas2) return tmpsign*can_swap_sum_sum(one,two,ignore_implicit_indices);
	if(comap1)           return tmpsign*can_swap_prod_obj(one,two,ignore_implicit_indices);
	if(comap2)           return tmpsign*can_swap_prod_obj(two,one,ignore_implicit_indices);
	if(comas1)           return tmpsign*can_swap_sum_obj(one,two,ignore_implicit_indices);
	if(comas2)           return tmpsign*can_swap_sum_obj(two,one,ignore_implicit_indices);
	
	return 1; // default: commuting.
	}

bool exptree_comparator::satisfies_conditions(exptree::iterator conditions, std::string& error) 
	{
	for(unsigned int i=0; i<exptree::arg_size(conditions); ++i) {
		exptree::iterator cond=exptree::arg(conditions, i);
		if(*cond->name=="\\unequals") {
			exptree::sibling_iterator lhs=cond.begin();
			exptree::sibling_iterator rhs=lhs;
			++rhs;
			// Lookup the replacement rules for the two given objects, and return true if 
			// those rules give a different result. But first check that there are rules
			// to start with.
//			std::cerr << *lhs->name  << " !=? " << *rhs->name << std::endl;
			if(replacement_map.find(exptree(lhs))==replacement_map.end() ||
				replacement_map.find(exptree(rhs))==replacement_map.end()) return true;
//			std::cerr << *lhs->name  << " !=?? " << *rhs->name << std::endl;
			if(tree_exact_equal(replacement_map[exptree(lhs)], replacement_map[exptree(rhs)])) {
				return false;
				}
			}
		else if(*cond->name=="\\indexpairs") {
			int countpairs=0;
			replacement_map_t::const_iterator it=replacement_map.begin(),it2;
			while(it!=replacement_map.end()) {
				it2=it;
				++it2;
				while(it2!=replacement_map.end()) {
					if(tree_exact_equal(it->second, it2->second)) {
						++countpairs;
						break;
						}
					++it2;
					}
				++it;
				}
//			txtout << countpairs << " pairs" << std::endl;
			if(countpairs!=*(cond.begin()->multiplier))
				return false;
			}
		else if(*cond->name=="\\regex") {
//			txtout << "regex matching..." << std::endl;
			exptree::sibling_iterator lhs=cond.begin();
			exptree::sibling_iterator rhs=lhs;
			++rhs;
			// If we have a match, all indices have replacement rules.
			std::string pat=(*rhs->name).substr(1,(*rhs->name).size()-2);
//			txtout << "matching " << *comp.replacement_map[lhs->name]
//					 << " with pattern " << pat << std::endl;
			pcrecpp::RE reg(pat);
			if(reg.FullMatch(*(replacement_map[exptree(lhs)].begin()->name))==false)
				return false;
			}
		else if(*cond->name=="\\hasprop") {
			exptree::sibling_iterator lhs=cond.begin();
			exptree::sibling_iterator rhs=lhs;
			++rhs;
			properties::registered_property_map_t::iterator pit=
				kernel.properties.registered_properties.store.find(*rhs->name);
			if(pit==kernel.properties.registered_properties.store.end()) {
				std::ostringstream str;
				str << "Property \"" << *rhs->name << "\" not registered." << std::endl;
				error=str.str();
				return false;
				}
			const property_base *aprop=pit->second();

			subtree_replacement_map_t::iterator subfind=subtree_replacement_map.find(lhs->name);
			replacement_map_t::iterator         patfind=replacement_map.find(exptree(lhs));

			if(subfind==subtree_replacement_map.end() && patfind==replacement_map.end()) {
				std::ostringstream str;
				str << "Pattern " << *lhs->name << " in \\hasprop did not occur in match." << std::endl;
				delete aprop;
				error=str.str();
				return false;
				}
			
			bool ret=false;
			if(subfind==subtree_replacement_map.end()) 
				 ret=kernel.properties.has(aprop, (*patfind).second.begin());
			else
				 ret=kernel.properties.has(aprop, (*subfind).second);
			delete aprop;
			return ret;
			}
		else {
			std::ostringstream str;
			str << "substitute: condition involving " << *cond->name << " not understood." << std::endl;
			error=str.str();
			return false;
			}
		}
	return true;
	}

bool exptree_is_equivalent::operator()(const exptree& one, const exptree& two)
	{
	int ret=subtree_compare(one.begin(), two.begin());
	if(ret==0) return true;
	else       return false;
	}

bool exptree_is_less::operator()(const exptree& one, const exptree& two)
	{
	int ret=subtree_compare(one.begin(), two.begin());
	if(ret < 0) return true;
	else        return false;
	}
