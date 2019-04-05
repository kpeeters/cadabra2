
#pragma once

#include "Storage.hh"
#include "Props.hh"
#include "properties/Indices.hh"

namespace cadabra {

	/// \ingroup compare
	///
	/// Basic building block subtree comparison function for tensors
	/// without dummy indices, which determines whether two tensor
	/// subtrees are equal up to the names of indices. This uses NO
	/// property information whatsoever; when indices are compared,
	/// they are simply compared based on their name, not on the
	/// index set they may belong to. In most cases, this is NOT what
	/// you want.
	///
	/// In MOST cases, the use of Ex_comparator below is recommended
	/// instead, as it can do much more complicated matching and will
	/// also keep track of the relation between symbols in the pattern
	/// and symbols in the object to be matched.
	///
	/// Examples:
	///
	///     A_m                        equals  A_n                       up to index names
	///     \diff{A*B_g*\diff{C}_k}_m  equals  \diff{A*B_h*\diff{C}_r}_s up to index names
	///
	///  return | meaning
	///  -------|-----------------------------------------------
	///  0      | structure equal, and all indices the same name
	///  1      | structure equal, index names of one < two
	/// -1      | structure equal, index names of one > two
	///  2      | structure different, one < two
	/// -2      | structure different, one > two
	///
	/// @param mod_prel            see below
	/// @param checksets           ignored FIXME: remove
	/// @param literal_wildcards   whether to treat wildcard names as ordinary names.
	///
	/// The mod_prel variable determines whether parent relations are taken into
	/// account when comparing:
	/// FIXME: now that subtree_compare does not use properties anymore, a lot can be simplified.
	///
	///        -3: require that parent relations match, unless indexpos=free.
	///        -2: require that parent relations match (even when indexpos = free)
	///        -1: do not require that parent relations match
	///       >=0: do not require parent relations to match up to and including this level
	///
	/// Similar logic holds for the compare_multiplier parameter.

	int subtree_compare(const Properties*,
	                    Ex::iterator one, Ex::iterator two,
	                    int mod_prel=-2, bool checksets=true, int compare_multiplier=-2,
	                    bool literal_wildcards=false);

	/// Various comparison functions, some exact, some with pattern logic.

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

	/// Compare for indexmap_t. The only comparator object that does not use
	/// properties info to lookup properties. This one compares exactly (it cannot
	/// do any matching which requires knowledge about index sets because it does
	/// not know about properties). It requires parent relations to match including
	/// at top level. It requires multipliers to match including at top level.
	/// Names with '?' or '??' suffixes are matched literally, not as patterns.

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

			enum class match_t {
				node_match=0,                 // a single node matches
				subtree_match=1,              // identical match, including index names
				match_index_less=2,           // structure match, indices in same set, but different names
				match_index_greater=3,
				no_match_indexpos_less=4,     // mismatch but only for index positions
				no_match_indexpos_greater=5,
				no_match_less=6,              // more serious mismatch
				no_match_greater=7
				};

			enum class useprops_t {
				always=0,         // always use property info
				not_at_top,       // don't use property info at top level of the expression
				never=2           // never use property info
				};

			/// Reset the object for a new match.

			void    clear();

			/// Determine whether Coordinates in the pattern (first argument
			/// to functions below) can match against Indices in the object
			/// (second argument). That is to say, whether the pattern
			/// `\partial_{t}{A}` matches against the expression
			/// `\partial_{\mu}{A}` when `\mu` can take the value `t`. This is
			/// used in 'evaluate', but should generically be turned off for
			/// 'substitute'.

			void    set_value_matches_index(bool);

			/// Match two subtrees taking into account symbol
			/// properties. 'i1' can be a pattern.
			/// Returns subtree_match or one of the no_match
			/// results.  You need to fill lhs_contains_dummies before
			/// calling!
			/// If use_props is false, it will not try to fetch any property
			/// information at the TOP level of the comparison. Properties
			/// will always be used at  levels.

			match_t equal_subtree(Ex::iterator i1, Ex::iterator i2,
			                      useprops_t use_props=useprops_t::always, bool ignore_parent_rel=false);

			/// Match two subtrees, new-style equal_subtree that handles conditions; this is
			/// what substitute uses.

			match_t match_subtree(const Ex&, Ex::iterator i1, Ex::iterator i2, Ex::iterator conditions);

			/// Find a sub-product in a product. The 'lhs' iterator points to the product which
			/// we want to find, the 'tofind' iterator to the current factor which we are looking
			/// for. The product in which to search is pointed to by 'st'.
			/// Once 'tofind' is found, this routine calls itself to find the next factor in
			/// 'lhs'. If the next factor cannot be found, we backtrack and try to find the
			/// previous factor again (it may have appeared multiple times).

			match_t match_subproduct(const Ex&,
			                         Ex::sibling_iterator lhs, Ex::sibling_iterator tofind,
			                         Ex::sibling_iterator st, Ex::iterator conditions);

			/// Find a sub-sum in a sum. The 'lhs' iterator points to the sum which
			/// we want to find, the 'tofind' iterator to the current term which we are looking
			/// for. The sum in which to search is pointed to by 'st'.
			/// Once 'tofind' is found, this routine calls itself to find the next term in
			/// 'lhs'. Since Cadabra assumes all terms in a sum commute, we do not
			/// need the backtracking logic of subproduct.

			match_t match_subsum(const Ex&,
			                     Ex::sibling_iterator lhs, Ex::sibling_iterator tofind,
			                     Ex::sibling_iterator st, Ex::iterator conditions);

			/// Check whether the a match found by calling equal_subtree or match_subproduct
			/// satisfies the conditions as stated.
			/// FIXME: document possible conditions.

			bool    satisfies_conditions(Ex::iterator conditions, std::string& error);

			/// Map for the replacement of nodes (indices, patterns).

			typedef std::map<Ex, Ex, tree_exact_less_no_wildcards_obj>     replacement_map_t;
			replacement_map_t                                              replacement_map;

			/// Map for the replacement of entire subtrees (object patterns).

			typedef std::map<nset_t::iterator, Ex::iterator, nset_it_less> subtree_replacement_map_t;
			subtree_replacement_map_t                                      subtree_replacement_map;

			/// Map for matching of index names to index values. Note: this is in the opposite order
			/// from replacement_map!

			replacement_map_t                                              index_value_map;

			/// Information to keep track of where individual factors/terms
			/// in a sub-product/sub-sum were found, and (for sub-products)
			/// whether moving them into the searched-for order leads to
			/// sign flips.

			std::vector<Ex::sibling_iterator> factor_locations;
			std::vector<int>                  factor_moving_signs;
			multiplier_t                      term_ratio;

			/// Flag to indicate whether additional care must be taken to handle dummies in the
			/// lhs of the pattern.
			/// FIXME: would be better if this were automatic.
			bool lhs_contains_dummies;

			/// Determine whether two objects should be swapped according to
			/// the available SortOrder properties.

			bool should_swap(Ex::iterator obj, match_t subtree_comparison) ;

			/// Determine whether obj and obj+1 be exchanged? If yes, return
			/// the sign, if no return zero. This is the general entry point
			/// for two arbitrary nodes (which may be a product or sum).
			///
			/// The last flag ('ignore_implicit_indices') is used to disable
			/// all checks dealing with implicit indices (this is useful for
			/// algorithms which re-order objects with implicit indices,
			/// which would otherwise always receive a 0 from this
			/// function).

			int  can_swap(Ex::iterator one, Ex::iterator two, match_t subtree_comparison,
			              bool ignore_implicit_indices=false);

			/// Determine whether object 'one' and 'two' can be moved next
			/// to each other by moving either one or the other: if fix_one==true
			/// the first node is kept fixed, otherwise the second node is kept fixed.

			int  can_move_adjacent(Ex::iterator prod, Ex::sibling_iterator one,
			                       Ex::sibling_iterator two, bool fix_one=false) ;


			/// Alternative to the above, which handles more complicated versions where we
			/// need to keep track of previously moved factors (used by algorithms/substitute.cc).

			int  can_move_adjacent(Ex::iterator prod, const std::vector<Ex::sibling_iterator>& factors, Ex::sibling_iterator to_move);

		protected:
			const Properties& properties;

			bool value_matches_index;

			/// Internal entry point. This comparison function tries to match
			/// a single node in the tree, except when the node is an
			/// index. Indices are considered to be leaf-nodes, and for these
			/// a full subtree match will be attempted (using subtree_compare).

			match_t compare(const Ex::iterator&, const Ex::iterator&,
			                bool       nobrackets=false,
			                useprops_t use_props=useprops_t::always,
			                bool       ignore_parent_rel=false);

			// Internal functions used by can_swap.
			int  can_swap_prod_obj(Ex::iterator prod, Ex::iterator obj, bool) ;
			int  can_swap_prod_prod(Ex::iterator prod1, Ex::iterator prod2, bool) ;
			int  can_swap_sum_obj(Ex::iterator sum, Ex::iterator obj, bool) ;
			int  can_swap_prod_sum(Ex::iterator prod, Ex::iterator sum, bool) ;
			int  can_swap_sum_sum(Ex::iterator sum1, Ex::iterator sum2, bool) ;
			int  can_swap_ilist_ilist(Ex::iterator obj1, Ex::iterator obj2);
			bool can_swap_different_indexsets(Ex::iterator obj1, Ex::iterator obj2);

			std::string tab() const;
			match_t     report(match_t r) const;

			static int offset;
		};

	/// \ingroup core
	///
	/// Basic comparison operator for tree iterators, so we can use them as keys in maps.

	class Ex_is_equivalent {
		public:
			Ex_is_equivalent(const Properties&);
			bool operator()(const Ex&, const Ex&);
		private:
			const Properties& properties;
		};

	class Ex_is_less {
		public:
			Ex_is_less(const Properties&, int mod_prel);
			bool operator()(const Ex&, const Ex&);
		private:
			const Properties& properties;
			int mod_prel;
		};



	}

bool operator<(const cadabra::Ex::iterator&, const cadabra::Ex::iterator&);
bool operator<(const cadabra::Ex&, const cadabra::Ex&);
std::ostream& operator<<(std::ostream&, cadabra::Ex_comparator::useprops_t up);
