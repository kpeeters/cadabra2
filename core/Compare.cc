
#include <typeinfo>

#include "Compare.hh"
#include "Algorithm.hh" // FIXME: only needed because index_iterator is in there
#include <sstream>
#include "pcrecpp.h"
#include "properties/Indices.hh"
#include "properties/Coordinate.hh"
#include "properties/ImplicitIndex.hh"
#include "properties/SelfCommutingBehaviour.hh"
#include "properties/CommutingAsSum.hh"
#include "properties/CommutingAsProduct.hh"
#include "properties/CommutingBehaviour.hh"
#include "properties/SortOrder.hh"

int subtree_compare(const Properties *properties, 
						  Ex::iterator one, Ex::iterator two, 
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
		if(checksets && properties!=0) {
			// Strip off the parent_rel because Indices properties are declared without
			// those.
			const Indices *ind1=properties->get<Indices>(one, true);
			const Indices *ind2=properties->get<Indices>(two, true);
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
	
//	std::cout << "mult for " << *one->name << " vs " << *two->name << " now " << mult << std::endl;

	// Compare sub/superscript relations.
	if((mod_prel==-2 /* && position_type!=Indices::free */) && one->is_index() && two->is_index() ) {
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

//	std::cout << "update: mult for " << *one->name << " vs " << *two->name << " now " << mult << std::endl;

	// Now turn to the child nodes. Before comparing them directly, first compare
	// the number of children, taking into account range wildcards.
	int numch1=Ex::number_of_children(one);
	int numch2=Ex::number_of_children(two);

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
	Ex::sibling_iterator sib1=one.begin(), sib2=two.begin();
	int remember_ret=0;
	if(mod_prel==0) mod_prel=-2;
	else if(mod_prel>0)  --mod_prel;
	if(compare_multiplier==0) compare_multiplier=-2;
	else if(compare_multiplier>0)  --compare_multiplier;

	while(sib1!=one.end()) {
		int ret=subtree_compare(properties, sib1,sib2, mod_prel, checksets, compare_multiplier, literal_wildcards);
		if(abs(ret)>1)
			return ret/abs(ret)*mult;
		if(ret!=0 && remember_ret==0) 
			remember_ret=ret;
		++sib1;
		++sib2;
		}
	return remember_ret;
	}

bool tree_less(const Properties* properties, const Ex& one, const Ex& two, int mod_prel, bool checksets, int compare_multiplier)
	{
	return subtree_less(properties, one.begin(), two.begin(), mod_prel, checksets, compare_multiplier);
	}

bool tree_equal(const Properties* properties, const Ex& one, const Ex& two, int mod_prel, bool checksets, int compare_multiplier)
	{
	return subtree_equal(properties, one.begin(), two.begin(), mod_prel, checksets, compare_multiplier);
	}

bool tree_exact_less(const Properties* properties, const Ex& one, const Ex& two, int mod_prel, bool checksets, int compare_multiplier, bool literal_wildcards)
	{
	return subtree_exact_less(properties, one.begin(), two.begin(), mod_prel, checksets, compare_multiplier, literal_wildcards);
	}

bool tree_exact_equal(const Properties* properties, const Ex& one, const Ex& two, int mod_prel, bool checksets, int compare_multiplier, bool literal_wildcards)
	{
	return subtree_exact_equal(properties, one.begin(), two.begin(), mod_prel, checksets, compare_multiplier, literal_wildcards);
	}

bool subtree_less(const Properties* properties, Ex::iterator one, Ex::iterator two, int mod_prel, bool checksets, int compare_multiplier)
	{
	int cmp=subtree_compare(properties, one, two, mod_prel, checksets, compare_multiplier);
	if(cmp==2) return true;
	return false;
	}

bool subtree_equal(const Properties* properties, Ex::iterator one, Ex::iterator two, int mod_prel, bool checksets, int compare_multiplier)
	{
	int cmp=subtree_compare(properties, one, two, mod_prel, checksets, compare_multiplier);
	if(abs(cmp)<=1) return true;
	return false;
	}

bool subtree_exact_less(const Properties* properties, Ex::iterator one, Ex::iterator two, int mod_prel, bool checksets, int compare_multiplier, bool literal_wildcards)
	{
	int cmp=subtree_compare(properties, one, two, mod_prel, checksets, compare_multiplier, literal_wildcards);
	if(cmp>0) return true;
	return false;
	}

bool subtree_exact_equal(const Properties* properties, Ex::iterator one, Ex::iterator two, int mod_prel, bool checksets, int compare_multiplier, bool literal_wildcards)
	{
	int cmp=subtree_compare(properties, one, two, mod_prel, checksets, compare_multiplier, literal_wildcards);
//	std::cout << *one->name << " == " << *two->name << " : " << cmp << std::endl;
	if(cmp==0) return true;
	return false;
	}

tree_less_obj::tree_less_obj(const Properties* k)
   : properties(k)
	{
	}

bool tree_less_obj::operator()(const Ex& one, const Ex& two) const
	{
	return tree_less(properties, one, two);
	}

tree_less_modprel_obj::tree_less_modprel_obj(const Properties* k)
   : properties(k)
	{
	}

bool tree_less_modprel_obj::operator()(const Ex& one, const Ex& two) const
	{
	return tree_less(properties, one, two, 0);
	}

tree_equal_obj::tree_equal_obj(const Properties* k)
   : properties(k)
	{
	}

bool tree_equal_obj::operator()(const Ex& one, const Ex& two) const
	{
	return tree_equal(properties, one, two);
	}

tree_exact_less_obj::tree_exact_less_obj(const Properties *p)
	: properties(p)
	{
	}

bool tree_exact_less_obj::operator()(const Ex& one, const Ex& two) const
	{
	return tree_exact_less(properties, one, two);
	}

tree_exact_less_no_wildcards_obj::tree_exact_less_no_wildcards_obj()
	{
	properties=0;
	}

bool tree_exact_less_no_wildcards_obj::operator()(const Ex& one, const Ex& two) const
	{
	return tree_exact_less(properties, one, two, -2, true, 0, true);
	}

bool tree_exact_less_no_wildcards_mod_prel_obj::operator()(const Ex& one, const Ex& two) const
	{
	return tree_exact_less(properties, one, two, 0, true, -2, true);
	}

bool tree_exact_equal_obj::operator()(const Ex& one, const Ex& two) const
	{
	return tree_exact_equal(properties, one, two);
	}

tree_exact_less_mod_prel_obj::tree_exact_less_mod_prel_obj(const Properties *p)
	: properties(p)
	{
	}

bool tree_exact_less_mod_prel_obj::operator()(const Ex& one, const Ex& two) const
	{
	return tree_exact_less(properties, one, two, 0, true, -2, true);
	}

bool tree_exact_equal_mod_prel_obj::operator()(const Ex& one, const Ex& two) const
	{
	return tree_exact_equal(properties, one, two, 0, true, -2, true);
	}

bool tree_exact_less_for_indexmap_obj::operator()(const Ex& one, const Ex& two) const
	{
	return tree_exact_less(0, one, two, 0, true, -2, true);
	}

//bool operator==(const Ex& first, const Ex& second)
//	{
//	return tree_exact_equal(properties, first, second, 0, true, -2, true);
//	}
//

void Ex_comparator::clear()
	{
	replacement_map.clear();
	subtree_replacement_map.clear();
	index_value_map.clear();
	factor_locations.clear();
	factor_moving_signs.clear();
	}

Ex_comparator::match_t Ex_comparator::equal_subtree(Ex::iterator i1, Ex::iterator i2)
	{
	Ex::sibling_iterator i1end(i1);
	Ex::sibling_iterator i2end(i2);
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
				size_t num1=Ex::number_of_children(i1);
				size_t num2=Ex::number_of_children(i2);

				// TODO: this is where we should decide what to do with
				// nodes which have sibling wildcards. Make a
				// 'compare_siblings'. We also need a
				// siblings_replacement_map in which we can store a
				// sibling range for a given sibling wildcard symbol,
				// e.g. a... -> i j k.  This matching routine should walk
				// to the first non-range object in the pattern, and match
				// that. Once it is matched, it should store the resulting
				// map for any range object to the left. Then continue,
				// recursively, finding the second non-range object, and
				// again storing the range object map.  Once a no-match
				// comes back, pop the match objects from the stack and 
				// try to find the non-range object to the right of the match
				// reported earlier.

				// This slightly oversearches (we could keep track of how
				// many non-range objects are still to be matched in order
				// to restrict how far to the right a search should go), but in 
				// practise this is probably not relevant (and can always be added).

				if(num1 < num2)      return no_match_less;
				else if(num1 > num2) return no_match_greater;
				break;
				}
			case subtree_match:
				// If a match of the entire subtrees at i1 and i2 has been found,
				// we do not need to go down the child nodes of i1 and i2 anymore.
				i1.skip_children();
				i2.skip_children();
				break;
			}
		// Continue walking the tree downwards. 
		++i1;
		++i2;
		}

	return subtree_match;
	}

Ex_comparator::Ex_comparator(const Properties& k)
	: properties(k)
	{
	}

Ex_comparator::match_t Ex_comparator::compare(const Ex::iterator& one, 
																		  const Ex::iterator& two, 
																		  bool nobrackets) 
	{
	// nobrackets also implies 'no multiplier', i.e. 'toplevel'.
	// 'one' is the substitute pattern, 'two' the expression under consideration.
	
//	std::cerr << "matching " << *one->name << " to " << *two->name << std::endl;

	if(nobrackets==false && one->fl.bracket != two->fl.bracket) 
		return (one->fl.bracket < two->fl.bracket)?no_match_less:no_match_greater;

//	std::cerr << "one passed" << std::endl;

	// FIXME: this needs to be relaxed for position-free indices
//HERE
//	if(one->fl.parent_rel != two->fl.parent_rel)                
//		return (one->fl.parent_rel < two->fl.parent_rel)?no_match_less:no_match_greater;

//	std::cerr << "two passed" << std::endl;

	// Determine whether we are dealing with one of the pattern types.
	bool pattern=false;
	bool objectpattern=false;
	bool implicit_pattern=false; // anything in _{..} or ^{..} that is not an integer or coordinate
	bool is_index=false;
	bool is_sibling_pattern=false;
	bool is_coordinate=false;
	
	if(one->fl.bracket==str_node::b_none && one->is_index() ) 
		is_index=true;
	if(one->is_name_wildcard())
		pattern=true;
	else if(one->is_object_wildcard())
		objectpattern=true;
	else if(one->is_siblings_wildcard())
		is_sibling_pattern=true;
	else if(is_index && one->is_integer()==false) {
		// Things in _{..} or ^{..} are either indices (implicit patterns) or coordinates.
		const Coordinate *cdn1=properties.get<Coordinate>(one, true);
		if(cdn1==0)
			implicit_pattern=true;
		else
			is_coordinate=true;
		}
		
	// Various cases to be distinguished now:
	//   - match index pattern to object
   //   - match object pattern to object
   //   - match coordinate to index
	//   - everything else, which does not involve patterns/wildcards

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
		if(loc == replacement_map.end() && Ex::number_of_children(one)!=0) {
			Ex tmp1(one);
			tmp1.erase_children(tmp1.begin());
			loc = replacement_map.find(tmp1);
			tested_full=false;
			}

		if(loc!=replacement_map.end()) {
			// We constructed a replacement rule for this node already at an earlier
			// stage. Need to make sure that that rule is consistent with what we
			// found now.

			int cmp;

			// If this is an index/pattern, try to match the whole index/pattern.
			if(tested_full) 
				cmp=subtree_compare(&properties, (*loc).second.begin(), two, -2 /* KP: don't switch to -2 (kk.cdb fails) */); 
			else {
				Ex tmp2(two);
				tmp2.erase_children(tmp2.begin());
				cmp=subtree_compare(&properties, (*loc).second.begin(), tmp2.begin(), -2 /* KP: see above */); 
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
			// This index/pattern was not encountered earlier. If this node is an index, 
			// check that the index types in pattern and object agree (if known, otherwise assume they match)

			//std::cerr << "index check " << *one->name << " " << *two->name << std::endl;

 			const Indices *t1=properties.get<Indices>(one, true);
			const Indices *t2=properties.get<Indices>(two, true);

			// Check parent rel if it matters.
			if(t1==0 || t2==0 || (t1->position_type!=Indices::free && t2->position_type!=Indices::free))
				if(one->fl.parent_rel != two->fl.parent_rel)                
					return (one->fl.parent_rel < two->fl.parent_rel)?no_match_less:no_match_greater;

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

			// See the documentation of substitute::can_apply for details about 
			// how the replacement_map is supposed to work. In general, if we want
			// that e.g a found _{z} also leads to a replacement rule for (z), or 
			// if we want that a found _{z} also leads to a replacement for ^{z},
			// this needs to be added to the replacement map explicitly.

			//std::cerr << "storing " << one->fl.parent_rel << " -> " << two->fl.parent_rel << std::endl;
			replacement_map[one]=two;
			
 			// if this is an index, also store the pattern with the parent_rel flipped

 			if(one->is_index()) {
 				Ex cmptree1(one);
 				Ex cmptree2(two);
 				cmptree1.begin()->flip_parent_rel();
 				if(two->is_index())
 					cmptree2.begin()->flip_parent_rel();
				replacement_map[cmptree1]=cmptree2;
 				}
			
			// if this is a pattern and the pattern has a non-zero number of children,
			// also add the pattern without the children
			if(Ex::number_of_children(one)!=0) {
				Ex tmp1(one), tmp2(two);
				tmp1.erase_children(tmp1.begin());
				tmp2.erase_children(tmp2.begin());
				replacement_map[tmp1]=tmp2;
				}
			// and if this is a pattern also insert the one without the parent_rel
			if(one->is_name_wildcard()) {
				Ex tmp1(one), tmp2(two);
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
	else if(is_coordinate) { // Check if the coordinate can come from an index. INCOMPLETE FIXME
		const Indices *t2=properties.get<Indices>(two, true);
		if(t2) {
			// std::cerr << "coordinate " << *one->name << " versus index " << *two->name << std::endl;
			// Look through values attribute of Indices object to see if the 'two' index
			// can take the 'one' value.
			
			//for(auto& ex: t2->values) {
				//std::cerr << *(ex.begin()->name) << std::endl;
			//	}
			auto ivals = std::find_if(t2->values.begin(), t2->values.end(), 
											  [&](const Ex& a) {
												  if(subtree_compare(&properties, a.begin(), one, 0)==0) return true;
												  else return false;
											  });
			if(ivals!=t2->values.end()) {
				index_value_map[one]=two;
				//std::cerr << " can take this value" << std::endl;
				return node_match;
				} 
			else {
				//std::cerr << " cannot take this value" << std::endl;
				return no_match_less;
				}
			}
		else return no_match_less;
		}
	else { // object is not dummy nor objectpattern nor coordinate

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


Ex_comparator::match_t Ex_comparator::match_subproduct(Ex::sibling_iterator lhs, 
																					  Ex::sibling_iterator tofind, 
																					  Ex::sibling_iterator st)
	{
	replacement_map_t         backup_replacements(replacement_map);
	subtree_replacement_map_t backup_subtree_replacements(subtree_replacement_map);

	// 'Start' iterates over all factors, trying to find 'tofind'. It may happen that the
	// first match is such that the entire subproduct matches, but it may be that we have
	// to iterate 'start' over more factors (typically when non-commutative objects are
	// concerned).

	Ex::sibling_iterator start=st.begin();
	while(start!=st.end()) {

		// The factor 'tofind' can only be matched against a factor in the subproduct if we 
		// have not already previously matched part of the lhs to this factor. We check that first.

		if(std::find(factor_locations.begin(), factor_locations.end(), start)==factor_locations.end()) {  

			// Compare this factor with 'tofind'. 

			if(equal_subtree(tofind, start)==subtree_match) { // found factor

				// Verify that the factor found now can be moved next to
				// the previous factor, if applicable (nontrivial if
				// factors do not commute).

				int sign=1;
				if(factor_locations.size()>0) {
					sign=can_move_adjacent(st, factor_locations.back(), start);
					}
				if(sign==0) { // object found, but we cannot move it in the right order
					replacement_map=backup_replacements;
					subtree_replacement_map=backup_subtree_replacements;
					}
				else {
					factor_locations.push_back(start);
					factor_moving_signs.push_back(sign);
					
					Ex::sibling_iterator nxt=tofind; 
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
int Ex_comparator::can_move_adjacent(Ex::iterator prod,
													 Ex::sibling_iterator one, Ex::sibling_iterator two) 
	{
	assert(Ex::parent(one)==Ex::parent(two));
	assert(Ex::parent(one)==prod);

	// Make sure that 'one' points to the object which occurs first in 'prod'.
	bool onefirst=false;
	Ex::sibling_iterator probe=one;
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
		int es=subtree_compare(&properties, one,two);
		sign*=can_swap(one,two,es);
//		txtout << "swapping one and two: " << sign << std::endl;
		}

	if(sign!=0) {
		// Loop over all pair flips which are necessary to move one to the left of two.
		probe=one;
		++probe;
		while(probe!=two) {
			assert(probe!=prod.end());
			int es=subtree_compare(&properties, one,probe);
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
bool Ex_comparator::should_swap(Ex::iterator obj, int subtree_comparison) 
	{
	Ex::sibling_iterator one=obj, two=obj;
	++two;

	// Find a SortOrder property which contains both one and two.
	int num1, num2;
	const SortOrder *so1=properties.get_composite<SortOrder>(one,num1);
	const SortOrder *so2=properties.get_composite<SortOrder>(two,num2);

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
int Ex_comparator::can_swap_prod_obj(Ex::iterator prod, Ex::iterator obj, 
													 bool ignore_implicit_indices) 
	{
//	std::cout << "prod_obj " << *prod->name << " " << *obj->name << std::endl;
	// Warning: no check is made that prod is actually a product!
	int sign=1;
	Ex::sibling_iterator sib=prod.begin();
	while(sib!=prod.end()) {
		const Indices *ind1=properties.get_composite<Indices>(sib, true);
		const Indices *ind2=properties.get_composite<Indices>(obj, true);
		if(! (ind1!=0 && ind2!=0) ) { // If both objects are actually real indices, 
			                           // then we do not include their commutativity property
			                           // in the sign. This is because the routines that use
                                    // can_swap_prod_obj all test for such index-index 
                                    // swaps separately.
			int es=subtree_compare(&properties, sib, obj, 0);
//			std::cout << "  " << *sib->name << " " << *obj->name << " " << es << std::endl;
			sign*=can_swap(sib, obj, es, ignore_implicit_indices);
			if(sign==0) break;
			}
		++sib;
		}
	return sign;
	}

int Ex_comparator::can_swap_prod_prod(Ex::iterator prod1, Ex::iterator prod2, 
													 bool ignore_implicit_indices)  
	{
//	std::cout << "prod_prod " << *prod1->name << " " << *prod2->name;
	// Warning: no check is made that prod1,2 are actually products!
	int sign=1;
	Ex::sibling_iterator sib=prod2.begin();
	while(sib!=prod2.end()) {
		sign*=can_swap_prod_obj(prod1, sib, ignore_implicit_indices);
		if(sign==0) break;
		++sib;
		}
//	std::cout << "  -> " << sign << std::endl;
	return sign;
	}

int Ex_comparator::can_swap_sum_obj(Ex::iterator sum, Ex::iterator obj, 
													bool ignore_implicit_indices) 
	{
	// Warning: no check is made that sum is actually a sum!
	int sofar=2;
	Ex::sibling_iterator sib=sum.begin();
	while(sib!=sum.end()) {
		int es=subtree_compare(&properties, sib, obj);
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

int Ex_comparator::can_swap_prod_sum(Ex::iterator prod, Ex::iterator sum, 
													 bool ignore_implicit_indices) 
	{
	// Warning: no check is made that sum is actually a sum or prod is a prod!
	int sign=1;
	Ex::sibling_iterator sib=prod.begin();
	while(sib!=prod.end()) {
//		const Indices *ind=kernel.properties->get_composite<Indices>(sib);
//		if(ind==0) {
		sign*=can_swap_sum_obj(sum, sib, ignore_implicit_indices);
			if(sign==0) break;
//			}
		++sib;
		}
	return sign;
	}

int Ex_comparator::can_swap_sum_sum(Ex::iterator sum1, Ex::iterator sum2,
													bool ignore_implicit_indices) 
	{
	int sofar=2;
	Ex::sibling_iterator sib=sum1.begin();
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

int Ex_comparator::can_swap_ilist_ilist(Ex::iterator obj1, Ex::iterator obj2) 
	{
	int sign=1;

	index_iterator it1=index_iterator::begin(properties, obj1);
	while(it1!=index_iterator::end(properties, obj1)) {
		index_iterator it2=index_iterator::begin(properties, obj2);
		while(it2!=index_iterator::end(properties, obj2)) {
			// Only deal with real indices here, i.e. those carrying an Indices property.
			const Indices *ind1=properties.get_composite<Indices>(it1, true);
			const Indices *ind2=properties.get_composite<Indices>(it2, true);
			if(ind1!=0 && ind2!=0) {
				const CommutingBehaviour *com1 =properties.get_composite<CommutingBehaviour>(it1, true);
				const CommutingBehaviour *com2 =properties.get_composite<CommutingBehaviour>(it2, true);
				
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

int Ex_comparator::can_swap(Ex::iterator one, Ex::iterator two, int subtree_comparison,
										 bool ignore_implicit_indices) 
	{
//	std::cout << "can_swap " << *one->name << " " << *two->name << ignore_implicit_indices << std::endl;

	const ImplicitIndex *ii1 = properties.get_composite<ImplicitIndex>(one);
	const ImplicitIndex *ii2 = properties.get_composite<ImplicitIndex>(two);

	// When both objects carry an implicit index but the index lines are not connected,
	// we should not be using explicit commutation rules, as this would mess up the
	// index lines and make the expression meaningless.
	// FIXME: this would ideally make use of index and conjugate index lines.

//	const DiracBar *db2 = kernel.properties->get_composite<DiracBar>(two);
	if(! (ii1 && ii2 /* && db2 */) ) {

		// First of all, check whether there is an explicit declaration for the commutativity 
		// of these two symbols.
//		std::cout << *one->name << " explicit " << *two->name << std::endl;
		const CommutingBehaviour *com = properties.get_composite<CommutingBehaviour>(one, two, true);
		
		if(com) {
//			std::cout << typeid(com).name() << std::endl;
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
	const SelfCommutingBehaviour *sc1 =properties.get_composite<SelfCommutingBehaviour>(one, true);
	const SelfCommutingBehaviour *sc2 =properties.get_composite<SelfCommutingBehaviour>(two, true);
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

	const CommutingAsProduct *comap1 = properties.get_composite<CommutingAsProduct>(one);
	const CommutingAsProduct *comap2 = properties.get_composite<CommutingAsProduct>(two);
	const CommutingAsSum     *comas1 = properties.get_composite<CommutingAsSum>(one);
	const CommutingAsSum     *comas2 = properties.get_composite<CommutingAsSum>(two);
	
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

bool Ex_comparator::satisfies_conditions(Ex::iterator conditions, std::string& error) 
	{
	for(unsigned int i=0; i<Ex::arg_size(conditions); ++i) {
		Ex::iterator cond=Ex::arg(conditions, i);
		if(*cond->name=="\\unequals") {
			Ex::sibling_iterator lhs=cond.begin();
			Ex::sibling_iterator rhs=lhs;
			++rhs;
			// Lookup the replacement rules for the two given objects, and return true if 
			// those rules give a different result. But first check that there are rules
			// to start with.
//			std::cerr << *lhs->name  << " !=? " << *rhs->name << std::endl;
			if(replacement_map.find(Ex(lhs))==replacement_map.end() ||
				replacement_map.find(Ex(rhs))==replacement_map.end()) return true;
//			std::cerr << *lhs->name  << " !=?? " << *rhs->name << std::endl;
			if(tree_exact_equal(&properties, replacement_map[Ex(lhs)], replacement_map[Ex(rhs)])) {
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
					if(tree_exact_equal(&properties, it->second, it2->second)) {
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
			Ex::sibling_iterator lhs=cond.begin();
			Ex::sibling_iterator rhs=lhs;
			++rhs;
			// If we have a match, all indices have replacement rules.
			std::string pat=(*rhs->name).substr(1,(*rhs->name).size()-2);
//			txtout << "matching " << *comp.replacement_map[lhs->name]
//					 << " with pattern " << pat << std::endl;
			pcrecpp::RE reg(pat);
			if(reg.FullMatch(*(replacement_map[Ex(lhs)].begin()->name))==false)
				return false;
			}
		// V2: FIXME: re-enable searching for properties
//		else if(*cond->name=="\\hasprop") {
//			Ex::sibling_iterator lhs=cond.begin();
//			Ex::sibling_iterator rhs=lhs;
//			++rhs;
//			Properties::registered_property_map_t::iterator pit=properties->store.find(*rhs->name);
//			if(pit==properties->store.end()) {
//				std::ostringstream str;
//				str << "Property \"" << *rhs->name << "\" not registered." << std::endl;
//				error=str.str();
//				return false;
//				}
//			const property_base *aprop=pit->second();
//
//			subtree_replacement_map_t::iterator subfind=subtree_replacement_map.find(lhs->name);
//			replacement_map_t::iterator         patfind=replacement_map.find(Ex(lhs));
//
//			if(subfind==subtree_replacement_map.end() && patfind==replacement_map.end()) {
//				std::ostringstream str;
//				str << "Pattern " << *lhs->name << " in \\hasprop did not occur in match." << std::endl;
//				delete aprop;
//				error=str.str();
//				return false;
//				}
//			
//			bool ret=false;
//			if(subfind==subtree_replacement_map.end()) 
//				 ret=properties->has(aprop, (*patfind).second.begin());
//			else
//				 ret=properties->has(aprop, (*subfind).second);
//			delete aprop;
//			return ret;
//			}
		else {
			std::ostringstream str;
			str << "substitute: condition involving " << *cond->name << " not understood." << std::endl;
			error=str.str();
			return false;
			}
		}
	return true;
	}

Ex_is_equivalent::Ex_is_equivalent(const Properties& k)
	: properties(k)
	{
	}

bool Ex_is_equivalent::operator()(const Ex& one, const Ex& two)
	{
	int ret=subtree_compare(&properties, one.begin(), two.begin());
	if(ret==0) return true;
	else       return false;
	}

Ex_is_less::Ex_is_less(const Properties& k, int mp)
	: properties(k), mod_prel(mp)
	{
	}

bool Ex_is_less::operator()(const Ex& one, const Ex& two)
	{
	int ret=subtree_compare(&properties, one.begin(), two.begin(), mod_prel);
	if(ret < 0) return true;
	else        return false;
	}


bool operator<(const Ex::iterator& i1, const Ex::iterator& i2)
	{
	return i1.node < i2.node;
	}
