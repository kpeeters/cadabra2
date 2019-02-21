
#include <typeinfo>

#include "Compare.hh"
#include "Algorithm.hh" // FIXME: only needed because index_iterator is in there
#include <sstream>
#include <regex>
#include "properties/Indices.hh"
#include "properties/Coordinate.hh"
#include "properties/ImplicitIndex.hh"
#include "properties/SelfCommutingBehaviour.hh"
#include "properties/CommutingAsSum.hh"
#include "properties/CommutingAsProduct.hh"
#include "properties/CommutingBehaviour.hh"
#include "properties/DifferentialForm.hh"
#include "properties/DiracBar.hh"
#include "properties/Integer.hh"
#include "properties/SortOrder.hh"

// In order to enable/disable debug output, also flip the swith in 'report' below.
//#define DEBUG(ln) ln
#define DEBUG(ln)

namespace cadabra {

int Ex_comparator::offset=0;

int subtree_compare(const Properties *properties, 
						  Ex::iterator one, Ex::iterator two, 
						  int mod_prel, bool , int compare_multiplier, bool literal_wildcards) 
	{
//	std::cerr << "comparing " << Ex(one) << " with " << Ex(two) << " " << mod_prel << ", " << checksets << std::endl;

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

	int  mult=2;
	// Handle object wildcards and comparison
	if(!literal_wildcards) {
		if(one->is_object_wildcard() || two->is_object_wildcard())
			return 0;
		}

	// Handle mismatching node names.
	if(one->name!=two->name) {
//		std::cerr << *one->name << " != " << *two->name << std::endl;
		if(literal_wildcards) {
			if(*one->name < *two->name) return mult;
			else return -mult;
			}

		if( (one->is_autodeclare_wildcard() && two->is_numbered_symbol()) || 
			 (two->is_autodeclare_wildcard() && one->is_numbered_symbol()) ) {
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

	// Compare parent relations.
	if(mod_prel<=-2) {
		str_node::parent_rel_t p1=one->fl.parent_rel;
		str_node::parent_rel_t p2=two->fl.parent_rel;
		if(p1!=p2) {
			return (p1<p2)?2:-2;
			}
		}

	// Now turn to the child nodes. Before comparing them directly, first compare
	// the number of children, taking into account range wildcards.
	int numch1=Ex::number_of_children(one);
	int numch2=Ex::number_of_children(two);

	// FIXME: handle range wildcards as in Props.cc

	if(numch1!=numch2) {
		if(numch1<numch2) return 2;
		else return -2;
		}

	// Compare actual children. We run through this twice: first
	// consider all non-index children, then all index children. This
	// is because we want products of tensors which differ only by
	// index position to be sorted by the names of the tensors first,
	// not first by their index positions. Otherwise canonicalise cannot
	// do its raising/lowering job.

	bool do_indices=false;
	int remember_ret=0;
	if(mod_prel==0) mod_prel=-2;
	else if(mod_prel>0)  --mod_prel;
	if(compare_multiplier==0) compare_multiplier=-2;
	else if(compare_multiplier>0)  --compare_multiplier;
	
	for(;;) {
		Ex::sibling_iterator sib1=one.begin(), sib2=two.begin();
		
		while(sib1!=one.end()) {
			if(sib1->is_index() == do_indices) {
				int ret=subtree_compare(properties, sib1,sib2, mod_prel, true /* checksets */, 
												compare_multiplier, literal_wildcards);
				// std::cerr << "result " << ret << std::endl;
				if(abs(ret)>1)
					return ret/abs(ret)*mult;
				if(ret!=0 && remember_ret==0) 
					remember_ret=ret;
				}
			++sib1;
			++sib2;
			}

		if(remember_ret!=0) break;

		if(!do_indices) do_indices=true;
		else break;
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
	//std::cerr << "comparing " << Ex(one) << " with " << Ex(two) << " = " << cmp << std::endl;
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
	return tree_exact_less(0, one, two, -2 /* mod_prel, was 0 */, true, -2 /* compare_multiplier, was 0 */, true); 
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

Ex_comparator::match_t Ex_comparator::equal_subtree(Ex::iterator i1, Ex::iterator i2, 
																	 useprops_t use_props, bool ignore_parent_rel)
	{
	++offset;

	Ex::sibling_iterator i1end(i1);
	Ex::sibling_iterator i2end(i2);
	++i1end;
	++i2end;

	bool first_call=true;
	match_t worst_mismatch=match_t::subtree_match;
	int topdepth=Ex::depth(i1);

	while(i1!=i1end && i2!=i2end) {
		int curdepth=Ex::depth(i1);
		// std::cerr << tab() << "match at depth " << curdepth << std::endl;
		useprops_t up = use_props;
		if(use_props==useprops_t::not_at_top) {
			if(topdepth!=curdepth) up=useprops_t::always;
			else                   up=useprops_t::never;
			}
		match_t mm=compare(i1, i2, first_call, up, ignore_parent_rel);
//		DEBUG( std::cerr << "COMPARE " << *i1->name << ", " << *i2->name << " = " << static_cast<int>(mm) << std::endl; )
		first_call=false;
		switch(mm) {
			case match_t::no_match_indexpos_less:
			case match_t::no_match_indexpos_greater:
				worst_mismatch=mm;
				// FIXME: at the moment skipping children is the right thing to do
				// as we are assuming that the no_match_indexpos_... is the worst
				// thing that could have happened for this child subtree. See
				// also the FIXME below.
				i1.skip_children();
				i2.skip_children();
				break;
			case match_t::no_match_less:
			case match_t::no_match_greater:
				// As soon as we get a mismatch, return.
				return report(mm);
			case match_t::node_match: {
				size_t num1=Ex::number_of_children(i1);
				size_t num2=Ex::number_of_children(i2);

				if(num1==1 && i1.begin()->is_range_wildcard()) {
//				std::cerr << "comparing " << *i1->name << " with " << *i2->name << " " << num1 << " " << num2 << std::endl;
					return match_t::subtree_match;
					}
				
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

				if(num1 < num2)      return report(match_t::no_match_less);
				else if(num1 > num2) return report(match_t::no_match_greater);
				break;
				}
			case match_t::match_index_less:
			case match_t::match_index_greater:
				// If we have a match but different index names, remember this
				// mismatch, because it will determine our total result value.
				if(worst_mismatch==match_t::subtree_match)
					worst_mismatch=mm;
				// If indices by type but not name, we do not need to go
				// further down the tree. E.g. W_{\hat{\theta_1}} vs
				// W_{\hat{\theta_2}} compared at the index position. 
				// The 'compare' has done a comparison of the entire
				// tree of the indices, no need to go down.
				i1.skip_children();
				i2.skip_children();
				break;
			case match_t::subtree_match:
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

	return report(worst_mismatch);
	}

Ex_comparator::Ex_comparator(const Properties& k)
	: properties(k), value_matches_index(false)
	{
	}

void Ex_comparator::set_value_matches_index(bool v)
	{
	value_matches_index=v;
	}

std::string Ex_comparator::tab() const
	{
	std::string ret;
	for(int i=0; i<offset; ++i)
		ret+="   ";
	return ret;
	}

Ex_comparator::match_t Ex_comparator::report(Ex_comparator::match_t r) const
	{
	return r;

	std::cerr << tab() << "result = ";
	switch(r) {
		case match_t::node_match: 
			std::cerr << "node_match";
			break;
		case match_t::subtree_match: 
			std::cerr << "subtree_match";
			break;
		case match_t::match_index_less: 
			std::cerr << "match_index_less";
			break;
		case match_t::match_index_greater: 
			std::cerr << "match_index_greater";
			break;
		case match_t::no_match_indexpos_less: 
			std::cerr << "no_match_indexpos_less";
			break;
		case match_t::no_match_indexpos_greater: 
			std::cerr << "no_match_indexpos_greater";
			break;
		case match_t::no_match_less: 
			std::cerr << "no_match_less";
			break;
		case match_t::no_match_greater: 
			std::cerr << "no_match_greater";
			break;
		}
	std::cerr << std::endl;
	--offset;
	return r;
	}

Ex_comparator::match_t Ex_comparator::compare(const Ex::iterator& one, 
															 const Ex::iterator& two, 
															 bool nobrackets, 
															 useprops_t use_props, bool ignore_parent_rel) 
	{
	++offset;
	
	// nobrackets also implies 'no multiplier', i.e. 'toplevel'.
	// 'one' is the substitute pattern, 'two' the expression under consideration.
	
	DEBUG( std::cerr << tab() << "matching " << Ex(one) << tab() << "to " << Ex(two) << tab() << "using props = " << use_props << ", ignore_parent_rel = " << ignore_parent_rel << std::endl; )

	if(nobrackets==false && one->fl.bracket != two->fl.bracket) 
		return report( (one->fl.bracket < two->fl.bracket)?match_t::no_match_less:match_t::no_match_greater );

	// Determine whether we are dealing with one of the pattern types.
	bool pattern=false;
	bool objectpattern=false;
	bool implicit_pattern=false; // anything in _{..} or ^{..} that is not an integer or coordinate
	bool is_index=false;
	bool is_sibling_pattern=false;
	bool is_coordinate=false;
	bool is_number=false;
	
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
		const Coordinate *cdn1=0;
		if(use_props==useprops_t::always) {
			DEBUG( std::cerr << tab() << "is " << *one->name << " a coordinate?" << std::endl; );
			cdn1=properties.get<Coordinate>(one, true);
			DEBUG( std::cerr << tab() << cdn1 << std::endl; );
			}
			
		if(cdn1==0)
			implicit_pattern=true;
		else
			is_coordinate=true;
		}
	else if(one->is_integer()) 
		is_number=true;
		
	// Various cases to be distinguished now:
	//   - match index pattern to object
   //   - match object pattern to object
   //   - match coordinate to index
	//   - everything else, which does not involve patterns/wildcards

	if(pattern || implicit_pattern) { 

		// It is possible to match integers to implicit patterns (indices), provided
		// we know that the given index can take the found numerical value. 
		// See basic.cdb/test26 for a simple example. 
		// If we have a proper 'question mark' pattern, we always match to integers.

		if(two->is_rational() && implicit_pattern) {
			// Determine whether 'one' can take the value 'two'.
			//std::cerr << "**** can one take value two " << use_props  << std::endl;
			const Integer *ip = 0;
			if(use_props==useprops_t::always) {
				DEBUG( std::cerr << tab() << "is " << *one->name << " an integer?" << std::endl; );
				ip = properties.get<Integer>(one, true); // 'true' to ignore parent rel.
				DEBUG( std::cerr << tab() << ip << std::endl; );
				}

			if(ip==0) return report(match_t::no_match_less);

			bool lower_bdy=true, upper_bdy=true;
			multiplier_t from, to;
			if(ip->from.begin()==ip->from.end()) lower_bdy=false;
			else {
				if(!ip->from.begin()->is_rational()) return report(match_t::no_match_less);
				from = *ip->from.begin()->multiplier;
				}
			if(ip->to.begin()==ip->to.end())     upper_bdy=false;
			else {
				if(!ip->to.begin()->is_rational()) return report(match_t::no_match_less);
				to   = *ip->to.begin()->multiplier;
				}
			if((lower_bdy && *two->multiplier < from) || (upper_bdy && *two->multiplier > to))  
				return report(match_t::no_match_less);
			
			// std::cerr << tab() << Ex(one) << tab() << "can take value " << *two->multiplier << std::endl;
			}

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
				cmp=subtree_compare(&properties, (*loc).second.begin(), two, -2); 
			else {
				Ex tmp2(two);
				tmp2.erase_children(tmp2.begin());
				cmp=subtree_compare(&properties, (*loc).second.begin(), tmp2.begin(), -2); 
				}
         //			std::cerr << " pattern " << *two->name
         //						 << " should be " << *((*loc).second.begin()->name)  
         //						 << " because that's what " << *one->name 
         //						 << " was set to previously; result " << cmp << std::endl;

			if(cmp==0)      return report(match_t::subtree_match);
			else if(cmp>0)  return report(match_t::no_match_less);
			else            return report(match_t::no_match_greater);
			}
		else {
			// This index/pattern was not encountered earlier. If this node is an index, 
			// check that the index types in pattern and object agree (if known, 
			// otherwise assume they match).
			// If two is a rational, we have already checked that one can take this value.

			if(one->is_index()) {
				DEBUG( std::cerr << tab() << "object one is index" << std::endl; )

				const Indices *t1=0;
				const Indices *t2=0;
				if(use_props==useprops_t::always) {
					DEBUG( std::cerr << tab() << "is " << *one->name << " an index?" << std::endl; )
					t1=properties.get<Indices>(one, false);
					DEBUG( std::cerr << tab() << t1 << std::endl; );
					if(two->is_rational()==false) {
						DEBUG( std::cerr << tab() << "is " << *two->name << " an index?" << std::endl; )
						t2=properties.get<Indices>(two, false);
						DEBUG( std::cerr << tab() << t2 << std::endl; );
						// It is still possible that t2 is a Coordinate and
						// t1 an Index which can take the value of the
						// coordinate. This happens when 'm' is an index
						// taking values {t,...}, you declare X^{m} to have
						// a property and then later ask if X^{t} has that
						// property. In that case X^{m} is object 1 and
						// X^{t} object 2, and we end up here (NOT in the
						// branch with both exchanged, which happens when
						// evaluate tries to determine if a rule for X^{t}
						// applies to an expression X^{m}).
						if(t1!=0 && t2!=t1) {
							auto ivals = std::find_if(t1->values.begin(), t1->values.end(), 
															  [&](const Ex& a) {
																  if(subtree_compare(&properties, a.begin(), two, 0)==0) return true;
																  else return false;
															  });
							if(ivals!=t1->values.end()) 
								t2=t1;
							}
						}
					else
						t2=t1; // We already know 'one' can take the value 'two', so in a sense two is in the same set as one.
					}
				
				// Check parent rel if a) there is no Indices property for the indices, b) the index positions
				// are not free. Effectively this means that indices without property info get treated as
				// fixed-position indices.

				// FIXME: this is not entirely correct. If the indexpos does not match, going deeper may
				// still reveal that the mismatch is even worse.

				if(!ignore_parent_rel) 
					if(t1==0 || t2==0 || (t1->position_type!=Indices::free && t2->position_type!=Indices::free))
						if(one->fl.parent_rel != two->fl.parent_rel) {               
							DEBUG( std::cerr << tab() << "parent_rels not the same" << std::endl;);
							return report( (one->fl.parent_rel < two->fl.parent_rel)
												?match_t::no_match_indexpos_less:match_t::no_match_indexpos_greater );
							}
			
				// If both indices have no Indices property, compare them by name and pretend they are
				// both in the same Indices set.
				
				if(two->is_rational()==false && !(t1==0 && t2==0)) {
					if( (t1 || t2) && implicit_pattern ) {
						if(t1 && t2) {
							if((*t1).set_name != (*t2).set_name) {
								if((*t1).set_name < (*t2).set_name) return report(match_t::no_match_less);
								else                                return report(match_t::no_match_greater);
								}
							}
						else {
							// If we get here, 'one' or 'two' has an Indices property, and
							// the other one doesn't. So we do not know how to compare them,
							// except by name. Note that we should return something which is
							// symmetric if 'two' and 'one' had been exchanged!
							int dc = subtree_compare(&properties, one, two, 0);
							if(dc<0) return report(match_t::no_match_less);
							else     return report(match_t::no_match_greater);
							}
						}
					}
				}

			// See the documentation of substitute::can_apply for details about 
			// how the replacement_map is supposed to work. In general, if we want
			// that e.g a found _{z} also leads to a replacement rule for (z), or 
			// if we want that a found _{z} also leads to a replacement for ^{z},
			// this needs to be added to the replacement map explicitly.

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
			if( /* one->is_name_wildcard()  &&  */ one->is_index()) {
				//std::cerr << "storing pattern without pattern rel " << replacement_map.size() << std::endl;
				Ex tmp1(one), tmp2(two);
				tmp1.begin()->fl.parent_rel=str_node::p_none;
				tmp2.begin()->fl.parent_rel=str_node::p_none;
				// We do not want this pattern to be present already.
				auto fnd=replacement_map.find(tmp1);
				if(fnd!=replacement_map.end()) {
					throw InternalError("Attempting to duplicate replacement rule.");
					}
//				assert(replacement_map.find(tmp1)!=replacement_map.end());
				// std::cerr << "storing " << Ex(tmp1) << " -> " << Ex(tmp2) << std::endl;
				replacement_map[tmp1]=tmp2;
				}
			}
		
		// Return a match of the appropriate type. If we are comparing indices and
		// they matched because they are from the same set but do not have the same
		// name, we still need to let the caller know about this.
		if(is_index) {
			int xc = subtree_compare(0, one, two, -2);
			if(xc==0) return report(match_t::subtree_match);
			if(xc>0)  return report(match_t::match_index_less);
			return report(match_t::match_index_greater);
			}
		else return report(match_t::node_match);
		}
	else if(objectpattern) {
		subtree_replacement_map_t::iterator loc=subtree_replacement_map.find(one->name);
		if(loc!=subtree_replacement_map.end()) {
			return report(equal_subtree((*loc).second,two));
			}
		else subtree_replacement_map[one->name]=two;
		
		return report(match_t::subtree_match);
		}
	else if(is_coordinate || is_number) { // Check if the coordinate can come from an index. 
		const Indices *t2=0;
		if(use_props==useprops_t::always) {
			DEBUG( std::cerr << tab() << "is " << *two->name << " an index?" << std::endl; )
			t2=properties.get<Indices>(two, true);
			DEBUG( std::cerr << tab() << t2 << std::endl; );
			}

		if(value_matches_index && t2) {
			// std::cerr << "coordinate " << *one->name << " versus index " << *two->name << std::endl;
			// If the 'two' index type is fixed or independent, ensure
			// that the parent relation matches!

			if(!ignore_parent_rel)
				if(t2->position_type==Indices::fixed || t2->position_type==Indices::independent) {
					if( one->fl.parent_rel != two->fl.parent_rel ) {
						DEBUG( std::cerr << tab() << "parent_rels not the same" << std::endl;);
						return report( (one->fl.parent_rel < two->fl.parent_rel)
											?match_t::no_match_indexpos_less:match_t::no_match_indexpos_greater );
						}
					}

			// Look through values attribute of Indices object to see if the 'two' index
			// can take the 'one' value. 

			auto ivals = std::find_if(t2->values.begin(), t2->values.end(), 
											  [&](const Ex& a) {
												  if(subtree_compare(&properties, a.begin(), one, 0)==0) return true;
												  else return false;
											  });
			if(ivals!=t2->values.end()) {
				// Verify that the 'two' index has not already been matched to a value
				// different from 'one'.
				Ex t1(two), t2(two), o1(one), o2(one);
				t2.begin()->flip_parent_rel();
				o2.begin()->flip_parent_rel();
				auto prev1 = index_value_map.find(t1);
				auto prev2 = index_value_map.find(t2);
				if(prev1!=index_value_map.end() && ! (prev1->second==o1) ) {
//					std::cerr << "Previously 1 " << Ex(two) << " was " << Ex(prev1->second) << std::endl;
					return report(match_t::no_match_less);
					}
				if(prev2!=index_value_map.end() && ! (prev2->second==o2) ) {
//					std::cerr << "Previously 2 " << Ex(two) << " was " << Ex(prev2->second) << std::endl;
					return report(match_t::no_match_less);
					}
		  
				index_value_map[two]=one;
				return report(match_t::node_match);
				} 
			else {
				// The index 'one' is not known to be able to take value 'two'. So this is not
				// a match. Compare lexographically; this should be symmetric if one and two had
				// been exchanged.
				int dc = subtree_compare(&properties, one, two, 0);
				if(dc<0) return report(match_t::no_match_less);
				else     return report(match_t::no_match_greater);
				}
			}
		else {
			// What's left is the possibility that both indices are Coordinates, in which case they
			// need to match exactly.
			int cmp=subtree_compare(&properties, one, two, -2);
			if(cmp==0)      return report(match_t::subtree_match);
			else if(cmp>0)  return report(match_t::no_match_less);
			else            return report(match_t::no_match_greater);
			}
		}
	else { // object is not dummy nor objectpattern nor coordinate

		if(one->is_rational() && two->is_rational()) {
			if(one->multiplier!=two->multiplier) {
				if(*one->multiplier < *two->multiplier) return report(match_t::no_match_less);
				else                                    return report(match_t::no_match_greater);
				}
			else {
				// Equal numerical factors with different parent rel _never_ match, because
				// we cannot determine if index raising/lowering is allowed if we do not
				// know the bundle type.
				if(!ignore_parent_rel)
					if(one->fl.parent_rel != two->fl.parent_rel)                
						return report( (one->fl.parent_rel < two->fl.parent_rel)
											?match_t::no_match_indexpos_less:match_t::no_match_indexpos_greater );
				}
			}
		
		if(one->name==two->name) {
			if(nobrackets || (one->multiplier == two->multiplier) ) {
				if(ignore_parent_rel || one->fl.parent_rel==two->fl.parent_rel) return report( match_t::node_match );
				report( (one->fl.parent_rel < two->fl.parent_rel)
				        ?match_t::no_match_indexpos_less:match_t::no_match_indexpos_greater );
				}

			if(*one->multiplier < *two->multiplier) return report(match_t::no_match_less);
			else                                    return report(match_t::no_match_greater);
			}
		else {
			if( *one->name < *two->name ) return report(match_t::no_match_less);
			else                          return report(match_t::no_match_greater);
			}
		}
	
	assert(1==0); // should never be reached

	return report(match_t::no_match_less); 
	}


Ex_comparator::match_t Ex_comparator::match_subproduct(const Ex& tr, 
																		 Ex::sibling_iterator lhs, 
																		 Ex::sibling_iterator tofind, 
																		 Ex::sibling_iterator st,
																		 Ex::iterator conditions)
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

			auto match = equal_subtree(tofind, start);
			if(match==match_t::subtree_match || match==match_t::match_index_less || match==match_t::match_index_greater) {

				// The factor has been found. Verify that it can be moved
				// next to the previous factor, if applicable (nontrivial
				// if factors do not commute).

				int sign=1;
				if(factor_locations.size()>0) {
					DEBUG(std::cerr << "--- can move?     ---" << std::endl; )
					Ex_comparator comparator(properties);
					sign=comparator.can_move_adjacent(st, factor_locations.back(), start);
					DEBUG(std::cerr << "--- done can move ---" << std::endl; )					
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
						match_t res=match_subproduct(tr, lhs, nxt, st, conditions);
						if(res==match_t::subtree_match) return res;
						else {
//						txtout << tofind.node << "found factor useless " << start.node << std::endl;
							factor_locations.pop_back();
							factor_moving_signs.pop_back();
							replacement_map=backup_replacements;
							subtree_replacement_map=backup_subtree_replacements;
							}
						}
					else {
						// Found all factors in sub-product, now check the conditions.
						std::string error;
						if(conditions==tr.end()) return match_t::subtree_match;
						if(satisfies_conditions(conditions, error)) {
							return match_t::subtree_match;
							}
						else {
							factor_locations.pop_back();
							factor_moving_signs.pop_back();
							replacement_map=backup_replacements;
							subtree_replacement_map=backup_subtree_replacements;
							}
						}
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
	return match_t::no_match_less; // FIXME not entirely true
	}

Ex_comparator::match_t Ex_comparator::match_subsum(const Ex& tr, 
																	Ex::sibling_iterator lhs, 
																	Ex::sibling_iterator tofind, 
																	Ex::sibling_iterator st,
																	Ex::iterator conditions)
	{
	replacement_map_t         backup_replacements(replacement_map);
	subtree_replacement_map_t backup_subtree_replacements(subtree_replacement_map);

	// 'Start' iterates over all terms, trying to find 'tofind'. It may happen that the
	// first match is such that the entire sub-sum matches, but it may be that we have
	// to iterate 'start' over more terms (typically when relative factors of terms
	// do not match).

	Ex::sibling_iterator start=st.begin();
	while(start!=st.end()) {

		// The term 'tofind' can only be matched against a term in the sub-sum if we 
		// have not already previously matched part of the lhs to this term. We check that first.

		if(std::find(factor_locations.begin(), factor_locations.end(), start)==factor_locations.end()) {  

			// Compare this factor with 'tofind'. 

			auto match = equal_subtree(tofind, start);
			if(match==match_t::subtree_match || match==match_t::match_index_less || match==match_t::match_index_greater) {

				// The term has been found. If this is not the 1st term, verify
				// that the ratio of its multiplier to the multiplier of the search term
				// agrees with that ratio for the first term.

				if(tr.index(tofind)==0) {
					term_ratio = (*tofind->multiplier)/(*start->multiplier);
					}
				auto this_ratio = (*tofind->multiplier)/(*start->multiplier);
				if(this_ratio!=term_ratio) {
					replacement_map=backup_replacements;
					subtree_replacement_map=backup_subtree_replacements;
					}
				else {
					factor_locations.push_back(start);
					
					Ex::sibling_iterator nxt=tofind; 
					++nxt;
					if(nxt!=lhs.end()) {
						match_t res=match_subsum(tr, lhs, nxt, st, conditions);
						if(res==match_t::subtree_match) return res;
						else {
							factor_locations.pop_back();
							replacement_map=backup_replacements;
							subtree_replacement_map=backup_subtree_replacements;
							}
						}
					else {
						// Found all factors in sub-product, now check the conditions.
						std::string error;
						if(conditions==tr.end()) return match_t::subtree_match;
						if(satisfies_conditions(conditions, error)) {
							return match_t::subtree_match;
							}
						else {
							factor_locations.pop_back();
							replacement_map=backup_replacements;
							subtree_replacement_map=backup_subtree_replacements;
							}
						}
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
	return match_t::no_match_less; // FIXME not entirely true
	}
	
//Ex_comparator::match_t Ex_comparator::match_subsum(const Ex& tr, 
//																		 Ex::sibling_iterator lhs, 
//																		 Ex::sibling_iterator tofind, 
//																		 Ex::sibling_iterator st,
//																		 Ex::iterator conditions)
//	{
//	// 'Start' iterates over all terms, trying to find 'tofind'.
//
//	Ex::sibling_iterator start=st.begin();
//	while(start!=st.end()) {
//
//		// The term 'tofind' can only be matched against a term in the
//		// subsum if we have not already previously matched part of the
//		// lhs to this factor. We check that first.
//
//		if(std::find(factor_locations.begin(), factor_locations.end(), start)==factor_locations.end()) {  
//
//			// Compare this term with 'tofind'. 
//
//			auto match = equal_subtree(tofind, start);
//			if(match==match_t::subtree_match || match==match_t::match_index_less || match==match_t::match_index_greater) {
//				factor_locations.push_back(start);
//				
//				Ex::sibling_iterator nxt=tofind; 
//				++nxt;
//				if(nxt!=lhs.end()) {
//					match_t res=match_subsum(tr, lhs, nxt, st, conditions);
//					return res;
//					}
//				else {
//					// Found all factors in sub-product.
//					auto sib=lhs.begin();
//					for(size_t i=0; i<factor_locations.size(); ++i) {
//						std::cerr << *factor_locations[i]->multiplier << " (vs " << *sib->multiplier << "), ";
//						++sib;
//						}
//					std::cerr << std::endl;
//
//					// Now check the conditions.
//					std::string error;
//					if(conditions==tr.end()) return match_t::subtree_match;
//					if(satisfies_conditions(conditions, error)) {
//						return match_t::subtree_match;
//						}
//					}
//				}
//			}
//		++start;
//		}
//	return match_t::no_match_less;
//	}
	

int Ex_comparator::can_move_adjacent(Ex::iterator prod,
												 Ex::sibling_iterator one, Ex::sibling_iterator two, bool fix_one) 
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
		auto es=equal_subtree(one,two);
		sign*=can_swap(one,two,es);
//		txtout << "swapping one and two: " << sign << std::endl;
		}

	if(sign!=0) {
		// Loop over all pair flips which are necessary to move one to the left of two.
		// Keep one fixed if fix_one is true.

		if(fix_one) {
			probe=two;
			--probe;
			while(probe!=one) {
				auto es=equal_subtree(two,probe);
				// std::cerr << "swapping " << Ex(two) << " and " << Ex(probe) << std::endl;
				sign*=can_swap(two,probe,es);
				if(sign==0) break;
				--probe;
				}
			}
		else {
			probe=one;
			++probe;
			while(probe!=two) {
				assert(probe!=prod.end());
				auto es=equal_subtree(one,probe);
				// std::cerr << "swapping " << Ex(one) << " and " << Ex(probe) << std::endl;
				sign*=can_swap(one,probe,es);
				if(sign==0) break;
				++probe;
				}
			}
		}
	return sign;
	}



// Should obj and obj+1 be swapped, according to the SortOrder
// properties?
//
bool Ex_comparator::should_swap(Ex::iterator obj, match_t subtree_comparison) 
	{
	Ex::sibling_iterator one=obj, two=obj;
	++two;

	// If the two objects are the same up to index names, we do not care
	// whether it sits in a SortOrder list; just compare alpha.
	if(subtree_comparison==match_t::match_index_less)    return false;
	if(subtree_comparison==match_t::match_index_greater) return true;

	// Find a SortOrder property which contains both one and two.
	int num1, num2;
	const SortOrder *so1=properties.get_composite<SortOrder>(one,num1);
	const SortOrder *so2=properties.get_composite<SortOrder>(two,num2);

	if(so1==0 || so2==0 || so1!=so2) {
		// std::cerr << "No sort order between " << Ex(one) << " and " << Ex(two);
		report(subtree_comparison);
		// std::cerr <<  std::endl;
      // No explicit sort order known; use alpha sort.
		if(subtree_comparison==match_t::subtree_match)    return false;
		if(subtree_comparison==match_t::no_match_less)    return false;
		if(subtree_comparison==match_t::no_match_greater) return true;
		return false;
		}

	assert(so1==so2);
	return num1>num2;
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
			auto es=equal_subtree(sib, obj);
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
		auto es=equal_subtree(sib, obj);
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

bool Ex_comparator::can_swap_different_indexsets(Ex::iterator obj1, Ex::iterator obj2) 
	{
	std::set<const Indices *> index_sets1;
	// std::cerr << "Are " << obj1 << " and " << obj2 << " swappable?" << std::endl;

	index_iterator it1=index_iterator::begin(properties, obj1);
	while(it1!=index_iterator::end(properties, obj1)) {
		auto ind = properties.get<Indices>(it1, true);
		if(!ind) return false;
		index_sets1.insert(ind);
		++it1;
		}
	index_iterator it2=index_iterator::begin(properties, obj2);
	while(it2!=index_iterator::end(properties, obj2)) {
		auto ind = properties.get<Indices>(it2, true);
		if(!ind) return false;
		if(index_sets1.find(ind)!=index_sets1.end()) {
			// std::cerr << "NO" << std::endl;
			return false;
			}
		++it2;
		}
	// std::cerr << "YES" << std::endl;		
	return true;
	}

int Ex_comparator::can_swap(Ex::iterator one, Ex::iterator two, match_t ,
									 bool ignore_implicit_indices) 
	{
	// std::cerr << "can_swap " << *one->name << " " << *two->name << " " << ignore_implicit_indices << std::endl;

	// Explicitly declared commutation behaviour goes first.
	const CommutingBehaviour *com = properties.get_composite<CommutingBehaviour>(one, two, true);
	if(com) 
		return com->sign();


	// If both objects have implicit indices, we cannot swap the
	// objects because that would re-order the index line. The sole
	// exception is when these indices are explicitly stated to be in
	// different sets.
	
	const ImplicitIndex *ii1 = properties.get_composite<ImplicitIndex>(one);
	const ImplicitIndex *ii2 = properties.get_composite<ImplicitIndex>(two);
	if(!ignore_implicit_indices) {
		if(ii1) {
			if(ii1->explicit_form.size()==0) {
				if(ii2) return 0; // nothing known about explicit form
				}
			else one=ii1->explicit_form.begin();
			}
		if(ii2) {
			if(ii2->explicit_form.size()==0) {
				if(ii1) return 0; // nothing known about explicit form
				}
			else two=ii2->explicit_form.begin();
			}
		// Check that indices in one and two are in mutually exclusive sets.
		if(ii1 && ii2) 
			if(!can_swap_different_indexsets(one, two))
				return false;
		}

	// Differential forms in a product cannot be moved through each
	// other except when the degree of one of them is zero.  In a wedge
	// product, we can move them and potentially pick up a sign.
	
	const DifferentialFormBase *df1 = properties.get<DifferentialFormBase>(one);
	const DifferentialFormBase *df2 = properties.get<DifferentialFormBase>(two);

	if(df1 && df2) {
		if(df1->degree(properties,one).begin()->is_zero() || df2->degree(properties,two).begin()->is_zero())
			return 1;
		else {
			if(Ex::is_head(one) || *(Ex::parent(one)->name)=="\\wedge") {
				if(df1->degree(properties, one).is_rational()==false ||
					df2->degree(properties, two).is_rational()==false)
					return 0; // Cannot yet order forms with non-numerical degrees.
				long d1 = to_long(df1->degree(properties, one).to_rational());
				long d2 = to_long(df2->degree(properties, two).to_rational());
				if( (d1*d2) % 2 == 1) return -1;
				return 1;
				}
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
		else if(*cond->name=="\\greater" || *cond->name=="\\less") {
			Ex::sibling_iterator lhs=cond.begin();
			Ex::sibling_iterator rhs=lhs;
			++rhs;

			multiplier_t mlhs;
			if(lhs->is_rational()==false) {
				auto fnd=replacement_map.find(Ex(lhs));
				if(fnd!=replacement_map.end()) {
					auto tn=fnd->second.begin();
					if(tn->is_rational())
						mlhs=*tn->multiplier;
					else {
						error="Replacement not numerical.";
						return false;
						}
					}
				else { 
					error="Can only compare objects which evaluate to numbers.";
					return false;
					}
				}
			else mlhs=*lhs->multiplier;

			// FIXME: abstract into Storage
			multiplier_t mrhs;
			if(rhs->is_rational()==false) {
				auto fnd=replacement_map.find(Ex(rhs));
				if(fnd!=replacement_map.end()) {
					auto tn=fnd->second.begin();
					if(tn->is_rational())
						mrhs=*tn->multiplier;
					else { 
						error="Replacement not numerical.";
						return false;
						}
					}
				else { 
					error="Can only compare objects which evaluate to numbers.";
					return false;
					}
				}
			else mrhs=*rhs->multiplier;

			if(*cond->name=="\\greater" && mlhs <= mrhs) return false;
			if(*cond->name=="\\less"    && mlhs >= mrhs) return false;
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
			std::regex reg(pat);
			if (!std::regex_match(*(replacement_map[Ex(lhs)].begin()->name), reg))
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

}

bool operator<(const cadabra::Ex::iterator& i1, const cadabra::Ex::iterator& i2)
	{
	return i1.node < i2.node;
	}

bool operator<(const cadabra::Ex& e1, const cadabra::Ex& e2)
	{
	return e1.begin().node < e2.begin().node;
	}

std::ostream& operator<<(std::ostream& s, cadabra::Ex_comparator::useprops_t up)
	{
	switch(up) {
		case cadabra::Ex_comparator::useprops_t::always:     s << "always";     break;
		case cadabra::Ex_comparator::useprops_t::not_at_top: s << "not_at_top"; break;
		case cadabra::Ex_comparator::useprops_t::never:      s << "never";      break;
		}
	return s;
	}


