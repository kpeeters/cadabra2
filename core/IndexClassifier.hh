
#pragma once

#include "Kernel.hh"
#include "Storage.hh"
#include "Props.hh"
#include "Compare.hh"

/// Class to deal with index scanning and classification.

namespace cadabra {

	class IndexClassifier {

		public:
			IndexClassifier(const Kernel&);

			/// A map from a pattern to the position where it occurs in the tree. The comparator
			/// is such that we store indices exactly, apart from their multiplicative factor.
			/// This means that the index in A_{n} and in A_{-n} are stored in the same way,
			/// and one needs to lookup the expression in the tree to find this multiplier.
			/// See basic.cdb test 26 for an example that uses this.
			typedef std::multimap<Ex, Ex::iterator, tree_exact_less_for_indexmap_obj> index_map_t;

			/// A map from the position of each index to the sequential index.
			typedef std::map<Ex::iterator, int, Ex::iterator_base_less>    index_position_map_t;


			/// Routines to find and classify all indices in an expression, taking into account
			/// sums and products. Note that dummy indices do not always come in pairs, for
			/// instance in expressions like
			///            a_{m n} ( b^{n p} + q^{n p} ) .
			/// Similarly, free indices can appear multiple times, as in
			///            a_{m} + b_{m} .
			void     fill_index_position_map(Ex::iterator, const index_map_t&, index_position_map_t&) const;
			void     fill_map(index_map_t&, Ex::sibling_iterator, Ex::sibling_iterator) const;
			void     print_classify_indices(std::ostream&, Ex::iterator) const;

			/// Determine those indices in 'two' which have a name which is identical to
			/// an index name occurring in 'one'. Store these indices of 'two' in target.
			/// If 'move_out' is true, instead move both the indices in 'one' and 'two'
			/// (i.e. move instead of copy, and also store the 'one' index).
			/// One exception: numerical, coordinate and symbol indices are always kept in 'one'.

			void     determine_intersection(index_map_t& one, index_map_t& two, index_map_t& target,
			                                bool move_out=false) const;

			void     classify_add_index(Ex::iterator it, index_map_t& ind_free, index_map_t& ind_dummy) const;

			/// Classify indices bottom-up, that is, given a node, it goes up the tree to find
         /// all free and dummy indices in the product in which this node would end up if a full
         /// distribute would be done on the entire expression.

			void     classify_indices_up(Ex::iterator, index_map_t& ind_free, index_map_t& ind_dummy) const;

         /// Classify indices top-down, that is, finds the free indices and all dummy
         /// index pairs used in the full subtree below a given node.

			void     classify_indices(Ex::iterator, index_map_t& ind_free, index_map_t& ind_dummy) const;
			
			int      max_numbered_name_one(const std::string& nm, const index_map_t * one) const;
			int      max_numbered_name(const std::string&, const index_map_t *m1, const index_map_t *m2=0,
			                           const index_map_t *m3=0, const index_map_t *m4=0, const index_map_t *m5=0) const;
			Ex get_dummy(const list_property *, const index_map_t *m1, const index_map_t *m2=0,
			             const index_map_t *m3=0, const index_map_t *m4=0, const index_map_t *m5=0) const;
			Ex get_dummy(const list_property *, Ex::iterator) const;
			Ex get_dummy(const list_property *, Ex::iterator, Ex::iterator) const;

			bool index_in_set(Ex, const index_map_t *) const;
			void     dumpmap(std::ostream&, const index_map_t&) const;

			/// Find an index in the set, not taking into account index position.
			index_map_t::iterator find_modulo_parent_rel(Ex::iterator it, index_map_t& imap) const;

		protected:
			const Kernel& kernel;

		};

	}
