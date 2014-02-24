
#pragma once

#include "Storage.hh"
#include "Props.hh"

// Generic subtree comparison class, which uses property information
// from a kernel to determine whether two tensor subtrees are equal in
// the sense of e.g. "SelfCommuting", that is up to the names of
// indices. E.g.,
//
//   A_m                        equals  A_n
//   \diff{A*B_g*\diff{C}_k}_m  equals  \diff{A*B_h*\diff{C}_r}_s
//
//  0: structure equal, and all indices the same name
//  1: structure equal, index names of one < two
// -1: structure equal, index names of one > two
//  2: structure different, one < two
// -2: structure different, one > two
//
// Note that this routine does not handle the more complicated case in
// which there are dummy indices inside the subtrees; for those cases
// you need to use "exptree_compare" (which in turn uses subtree_compare
// to do the simpler cases).
//
// Parameters:
// literal_wildcards: if true, treat wildcard names as ordinary names.

int subtree_compare(const Properties*, 
						  exptree::iterator one, exptree::iterator two, 
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
					const exptree& one, const exptree& two, 
					int mod_prel=-2, bool checksets=true, int compare_multiplier=-2);
bool tree_equal(const Properties*, 
					 const exptree& one, const exptree& two, 
					 int mod_prel=-2, bool checksets=true, int compare_multiplier=-2);
bool tree_exact_less(const Properties*, 
							const exptree& one, const exptree& two, 
							int mod_prel=-2, bool checksets=true, int compare_multiplier=-2,
							bool literal_wildcards=false);
bool tree_exact_equal(const Properties*, 
							 const exptree& one, const exptree& two,
							 int mod_prel=-2, bool checksets=true, int compare_multiplier=-2,
							 bool literal_wildcards=false);

bool subtree_less(const Properties*, 
						exptree::iterator one, exptree::iterator two,
						int mod_prel=-2, bool checksets=true, int compare_multiplier=-2);
bool subtree_equal(const Properties*, 
						 exptree::iterator one, exptree::iterator two,
						 int mod_prel=-2, bool checksets=true, int compare_multiplier=-2);
bool subtree_exact_less(const Properties*, 
								exptree::iterator one, exptree::iterator two,
								int mod_prel=-2, bool checksets=true, int compare_multiplier=-2,
								bool literal_wildcards=false);
bool subtree_exact_equal(const Properties*, 
								 exptree::iterator one, exptree::iterator two,
								 int mod_prel=-2, bool checksets=true, int compare_multiplier=-2,
								 bool literal_wildcards=false);

/// Compare two trees by pattern logic, i.e. modulo index names.
//
class tree_less_obj {
	public:
		tree_less_obj(const Properties*);
		bool operator()(const exptree& first, const exptree& second) const;
	private:
		const Properties* properties;
};

class tree_less_modprel_obj {
	public:
		tree_less_modprel_obj(const Properties*);
		bool operator()(const exptree& first, const exptree& second) const;
	private:
		const Properties* properties;
};

class tree_equal_obj {
	public:
		tree_equal_obj(const Properties*);
		bool operator()(const exptree& first, const exptree& second) const;
	private:
		const Properties* properties;
};

/// Compare two trees exactly, i.e. including exact index names.
//
class tree_exact_less_obj {
	public:
		tree_exact_less_obj(const Properties*);
		bool operator()(const exptree& first, const exptree& second) const;
	private:
		const Properties* properties;
};

class tree_exact_less_mod_prel_obj {
	public:
		tree_exact_less_mod_prel_obj(const Properties*);
		bool operator()(const exptree& first, const exptree& second) const;
	private:
		const Properties* properties;
};

class tree_exact_equal_obj {
	public:
		tree_exact_equal_obj(const Properties*);
		bool operator()(const exptree& first, const exptree& second) const;
	private:
		const Properties* properties;
};

class tree_exact_equal_mod_prel_obj {
	public:
		tree_exact_equal_mod_prel_obj(const Properties*);
		bool operator()(const exptree& first, const exptree& second) const;
	private:
		const Properties* properties;
};

// Compare for indexmap_t. The only comparator object that does not use
// properties info to lookup properties.

class tree_equal_for_indexmap_obj {
	public:
		bool operator()(const exptree& first, const exptree& second) const;
};

/// Compare two trees exactly, treat wildcard names as ordinary names.
//
class tree_exact_less_no_wildcards_obj {
	public:
		tree_exact_less_no_wildcards_obj(); // disables property handling
		tree_exact_less_no_wildcards_obj(const Properties*);
		bool operator()(const exptree& first, const exptree& second) const;
	private:
		const Properties* properties;
};

class tree_exact_less_no_wildcards_mod_prel_obj {
	public:
		tree_exact_less_no_wildcards_mod_prel_obj(const Properties*);
		bool operator()(const exptree& first, const exptree& second) const;
	private:
		const Properties* properties;
};


/// This operator does an exact comparison, with no symbols interpreted
/// as wildcards or patterns.

//bool operator==(const exptree& first, const exptree& second);


/// A generic tree comparison class which will take into account index
/// contractions and will also keep track of a replacement list for
/// all types of cadabra wildcards.

class exptree_comparator {
	public:
		exptree_comparator(const Properties&);

		enum match_t { node_match=0, subtree_match=1, no_match_less=2, no_match_greater=3 };

		void    clear();

		// Subtree and subproduct match; return subtree_match or one of the no_match results.
		// You need to fill lhs_contains_dummies before calling!
		match_t equal_subtree(exptree::iterator i1, exptree::iterator i2);
		match_t match_subproduct(exptree::sibling_iterator lhs, exptree::sibling_iterator tofind, 
										 exptree::sibling_iterator st);
		bool    satisfies_conditions(exptree::iterator conditions, std::string& error);

		// Maps for replacement of nodes (indices, patterns) and subtrees (object patterns) respectively.
		typedef std::map<exptree, exptree, tree_exact_less_no_wildcards_obj>  replacement_map_t;
		typedef std::map<nset_t::iterator, exptree::iterator, nset_it_less>   subtree_replacement_map_t;

		replacement_map_t                      replacement_map;
		subtree_replacement_map_t              subtree_replacement_map;

		std::vector<exptree::sibling_iterator> factor_locations;
		std::vector<int>                       factor_moving_signs;

		bool lhs_contains_dummies;

      // A set of routines to determine natural orders of factors in products.
		// These used to be in exptree_ordering but were moved in v2 because these
		// all need properties info.

		bool should_swap(exptree::iterator obj, int subtree_comparison) ;
		int  can_swap_prod_obj(exptree::iterator prod, exptree::iterator obj, bool) ;
		int  can_swap_prod_prod(exptree::iterator prod1, exptree::iterator prod2, bool) ;
		int  can_swap_sum_obj(exptree::iterator sum, exptree::iterator obj, bool) ;
		int  can_swap_prod_sum(exptree::iterator prod, exptree::iterator sum, bool) ;
		int  can_swap_sum_sum(exptree::iterator sum1, exptree::iterator sum2, bool) ;
		int  can_swap_ilist_ilist(exptree::iterator obj1, exptree::iterator obj2);
		int  can_swap(exptree::iterator one, exptree::iterator two, int subtree_comparison,
						  bool ignore_implicit_indices=false);
		int  can_move_adjacent(exptree::iterator prod, 
									  exptree::sibling_iterator one, exptree::sibling_iterator two) ;

	protected:
		const Properties& properties;
		// Internal entry point. 
		match_t compare(const exptree::iterator&, const exptree::iterator&, bool nobrackets=false);
};

class exptree_is_equivalent {
	public:
		exptree_is_equivalent(const Properties&);
		bool operator()(const exptree&, const exptree&);
	private:
		const Properties& properties;
};

class exptree_is_less {
	public:
		exptree_is_less(const Properties&);
		bool operator()(const exptree&, const exptree&);
	private:
		const Properties& properties;
};



