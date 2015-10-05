
#pragma once

#include "Storage.hh"
#include "Props.hh"
#include "properties/Indices.hh"

/// \ingroup compare
///
/// Basic building block subtree comparison function for tensors
/// without dummy indices, which uses property information from a
/// kernel to determine whether two tensor subtrees are equal in the
/// sense of for instance the SelfCommuting property, that is, up to
/// the names of indices. Examples:
///
///     A_m                        equals  A_n
///     \diff{A*B_g*\diff{C}_k}_m  equals  \diff{A*B_h*\diff{C}_r}_s
///
///  return | meaning
///  -------|-----------------------------------------------
///  0      | structure equal, and all indices the same name
///  1      | structure equal, index names of one < two
/// -1      | structure equal, index names of one > two
///  2      | structure different, one < two
/// -2      | structure different, one > two
///
/// @param literal_wildcards whether to treat wildcard names as ordinary names.

int subtree_compare(const Properties*, 
						  Ex::iterator one, Ex::iterator two, 
						  int mod_prel=-2, bool checksets=true, int compare_multiplier=-2, 
						  bool literal_wildcards=false);

/// Various comparison functions, some exact, some with pattern logic.
/// The mod_prel variable determines whether parent relations are taken into
/// account when comparing:
///
///        -2: require that parent relations match (or that indices are position-free)
///        -1: do not require that parent relations match
///       >=0: do not require parent relations to match up to and including this level
///
/// Similar logic holds for the compare_multiplier parameter.
//
bool tree_less(const Properties*, 
					const Ex& one, const Ex& two, 
					int mod_prel=-2, bool checksets=true, int compare_multiplier=-2);
bool tree_equal(const Properties*, 
					 const Ex& one, const Ex& two, 
					 int mod_prel=-2, bool checksets=true, int compare_multiplier=-2);
bool tree_exact_less(const Properties*, 
							const Ex& one, const Ex& two, 
							int mod_prel=-2, bool checksets=true, int compare_multiplier=-2,
							bool literal_wildcards=false);
bool tree_exact_equal(const Properties*, 
							 const Ex& one, const Ex& two,
							 int mod_prel=-2, bool checksets=true, int compare_multiplier=-2,
							 bool literal_wildcards=false);

bool subtree_less(const Properties*, 
						Ex::iterator one, Ex::iterator two,
						int mod_prel=-2, bool checksets=true, int compare_multiplier=-2);
bool subtree_equal(const Properties*, 
						 Ex::iterator one, Ex::iterator two,
						 int mod_prel=-2, bool checksets=true, int compare_multiplier=-2);
bool subtree_exact_less(const Properties*, 
								Ex::iterator one, Ex::iterator two,
								int mod_prel=-2, bool checksets=true, int compare_multiplier=-2,
								bool literal_wildcards=false);
bool subtree_exact_equal(const Properties*, 
								 Ex::iterator one, Ex::iterator two,
								 int mod_prel=-2, bool checksets=true, int compare_multiplier=-2,
								 bool literal_wildcards=false);

/// Compare two trees by pattern logic, i.e. modulo index names.
//
class tree_less_obj {
	public:
		tree_less_obj(const Properties*);
		bool operator()(const Ex& first, const Ex& second) const;
	private:
		const Properties* properties;
};

class tree_less_modprel_obj {
	public:
		tree_less_modprel_obj(const Properties*);
		bool operator()(const Ex& first, const Ex& second) const;
	private:
		const Properties* properties;
};

class tree_equal_obj {
	public:
		tree_equal_obj(const Properties*);
		bool operator()(const Ex& first, const Ex& second) const;
	private:
		const Properties* properties;
};

/// Compare two trees exactly, i.e. including exact index names.
//
class tree_exact_less_obj {
	public:
		tree_exact_less_obj(const Properties*);
		bool operator()(const Ex& first, const Ex& second) const;
	private:
		const Properties* properties;
};

class tree_exact_less_mod_prel_obj {
	public:
		tree_exact_less_mod_prel_obj(const Properties*);
		bool operator()(const Ex& first, const Ex& second) const;
	private:
		const Properties* properties;
};

class tree_exact_equal_obj {
	public:
		tree_exact_equal_obj(const Properties*);
		bool operator()(const Ex& first, const Ex& second) const;
	private:
		const Properties* properties;
};

class tree_exact_equal_mod_prel_obj {
	public:
		tree_exact_equal_mod_prel_obj(const Properties*);
		bool operator()(const Ex& first, const Ex& second) const;
	private:
		const Properties* properties;
};

// Compare for indexmap_t. The only comparator object that does not use
// properties info to lookup properties.

class tree_exact_less_for_indexmap_obj {
	public:
		bool operator()(const Ex& first, const Ex& second) const;
};

/// Compare two trees exactly, treat wildcard names as ordinary names.
//
class tree_exact_less_no_wildcards_obj {
	public:
		tree_exact_less_no_wildcards_obj(); // disables property handling
		tree_exact_less_no_wildcards_obj(const Properties*);
		bool operator()(const Ex& first, const Ex& second) const;
	private:
		const Properties* properties;
};

class tree_exact_less_no_wildcards_mod_prel_obj {
	public:
		tree_exact_less_no_wildcards_mod_prel_obj(const Properties*);
		bool operator()(const Ex& first, const Ex& second) const;
	private:
		const Properties* properties;
};


/// \ingroup compare
///
/// A generic tree comparison class which will take into account index
/// contractions and will also keep track of a replacement list for
/// all types of cadabra wildcards. The entry point is typically 
/// 'equal_subtree' or 'match_subproduct'. 

class Ex_comparator {
	public:
		Ex_comparator(const Properties&);

		enum match_t { node_match=0, subtree_match=1, no_match_less=2, no_match_greater=3 };

		/// Reset the object for a new match.

		void    clear();

		/// Match two subtrees taking into account symbol
		/// properties. Return subtree_match or one of the no_match
		/// results.  You need to fill lhs_contains_dummies before
		/// calling!

		match_t equal_subtree(Ex::iterator i1, Ex::iterator i2);

      /// Find a subproduct in a product. The 'lhs' iterator points to the product which
      /// we want to find, the 'tofind' iterator to the current factor which we are looking
      /// for. The product in which to search is pointed to by 'st'.
      /// Once 'tofind' is found, this routine calls itself to find the next factor in
      /// 'lhs'. If the next factor cannot be found, we backtrack and try to find the
      /// previous factor again (it may have appeared multiple times).

		match_t match_subproduct(Ex::sibling_iterator lhs, Ex::sibling_iterator tofind, 
										 Ex::sibling_iterator st);


		/// Check whether the a match found by calling equal_subtree or match_subproduct 
		/// satisfies the conditions as stated.
		/// FIXME: document possible conditions.
		bool    satisfies_conditions(Ex::iterator conditions, std::string& error);

		/// Map for the replacement of nodes (indices, patterns).
		typedef std::map<Ex, Ex, tree_exact_less_no_wildcards_obj>  replacement_map_t;
		replacement_map_t                                                     replacement_map;

		/// Map for the replacement of entire subtrees (object patterns).
		typedef std::map<nset_t::iterator, Ex::iterator, nset_it_less>   subtree_replacement_map_t;
		subtree_replacement_map_t                                             subtree_replacement_map;

		/// Map for the association of indices with their values, in order to be able
		/// to match a pattern with component values, e.g. A_{t r}, to an expression containing
		/// an abstract tensor, e.g. A_{m n}. Is used in the compare method around the
		/// 'Check if the coordinate can come from an index' comment. Used for component
		/// evaluations.
		typedef std::map<const Indices *, Ex> index_value_map_t;
		index_value_map_t                          index_values;


		std::vector<Ex::sibling_iterator> factor_locations;
		std::vector<int>                       factor_moving_signs;

		bool lhs_contains_dummies;

      /// Determine whether two objects should be swapped according to
      /// the available SortOrder properties.

		bool should_swap(Ex::iterator obj, int subtree_comparison) ;

      /// Determine whether obj and obj+1 be exchanged? If yes, return
      /// the sign, if no return zero. This is the general entry point
      /// for two arbitrary nodes (which may be a product or sum).
      ///
      /// The last flag ('ignore_implicit_indices') is used to disable
      /// all checks dealing with implicit indices (this is useful for
      /// algorithms which re-order objects with implicit indices,
      /// which would otherwise always receive a 0 from this
      /// function).

		int  can_swap(Ex::iterator one, Ex::iterator two, int subtree_comparison,
						  bool ignore_implicit_indices=false);
		int  can_move_adjacent(Ex::iterator prod, 
									  Ex::sibling_iterator one, Ex::sibling_iterator two) ;

	protected:
		const Properties& properties;
		// Internal entry point. 
		match_t compare(const Ex::iterator&, const Ex::iterator&, bool nobrackets=false);

      // Internal functions used by can_swap.
		int  can_swap_prod_obj(Ex::iterator prod, Ex::iterator obj, bool) ;
		int  can_swap_prod_prod(Ex::iterator prod1, Ex::iterator prod2, bool) ;
		int  can_swap_sum_obj(Ex::iterator sum, Ex::iterator obj, bool) ;
		int  can_swap_prod_sum(Ex::iterator prod, Ex::iterator sum, bool) ;
		int  can_swap_sum_sum(Ex::iterator sum1, Ex::iterator sum2, bool) ;
		int  can_swap_ilist_ilist(Ex::iterator obj1, Ex::iterator obj2);
};

class Ex_is_equivalent {
	public:
		Ex_is_equivalent(const Properties&);
		bool operator()(const Ex&, const Ex&);
	private:
		const Properties& properties;
};

class Ex_is_less {
	public:
		Ex_is_less(const Properties&);
		bool operator()(const Ex&, const Ex&);
	private:
		const Properties& properties;
};



