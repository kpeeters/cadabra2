/* 

	Cadabra: a field-theory motivated computer algebra system.
	Copyright (C) 2001-2014  Kasper Peeters <kasper.peeters@phi-sci.com>

   This program is free software: you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
*/

#pragma once

#include "Stopwatch.hh"
#include "Storage.hh"
#include "Compare.hh"
#include "Props.hh"
#include "Exceptions.hh"
#include "Kernel.hh"
#include "IndexIterator.hh"

#include <map>
#include <fstream>
#include <cstddef>

// Base class for all algorithms, containing generic routines and in
// particular the logic for index classification. In particular also
// contains static algorithms acting on exptree objects which require
// property information and can therefore not be a member of exptree.

class Algorithm {
	public:
		// Initialise the algorithm with a reference to the expression tree;
		// does not do anything yet.
		Algorithm(Kernel&, exptree&);

		virtual ~Algorithm();

		typedef exptree::iterator            iterator;
		typedef exptree::post_order_iterator post_order_iterator;
		typedef exptree::sibling_iterator    sibling_iterator;

		bool interrupted;

		// TODO: make these capitalised names.

		enum class result_t {
				l_no_action,  // algorithm has modified the expression
				l_applied,    // algorithm left expression unchanged
				l_error       // algorithm left expression inconsistent due to unrecoverable error
		};


		// The main entry points for running algorithms. The 'deep' flag indicates
		// whether sub-expressions should be acted on too. The 'repeat' flag indicates
		// whether the algorithm should be applied until the expression no longer 
		// changes.
		result_t  apply_generic(iterator&, bool deep, bool repeat);

		// Global information
		unsigned int     number_of_calls;
		unsigned int     number_of_modifications;
		bool             suppress_normal_output;
		bool             discard_command_node;

		// Given an \expression node, check consistency
		bool      check_consistency(iterator) const;
		bool      check_index_consistency(iterator) const;

		void report_progress(const std::string&, int todo, int done, int count=2);

		mutable stopwatch index_sw;
		mutable stopwatch get_dummy_sw;
		mutable stopwatch report_progress_stopwatch;

		index_iterator begin_index(iterator it) const;
		index_iterator end_index(iterator it) const;

		// The number of indices of a node, taking into account IndexInherit-ance
		unsigned int number_of_indices(iterator it);
		static unsigned int number_of_indices(const Properties&, iterator it);
		
		// The number of indices of a node, counting only the direct ones (i.e. not those
		// inherited from child nodes).
		static unsigned int number_of_direct_indices(iterator it);

		bool     rename_replacement_dummies(iterator, bool still_inside_algo=false);
	protected:
		Kernel&  kernel;
		exptree& tr;

		// The main entry point which is used by the public entry points listed
		// above. Override these in any subclass.
		//
		virtual bool can_apply(iterator)=0;
		virtual result_t apply(iterator&)=0;

		// Index stuff
		int      index_parity(iterator) const;
		static bool less_without_numbers(nset_t::iterator, nset_t::iterator);
		static bool equal_without_numbers(nset_t::iterator, nset_t::iterator);
		
		/// Finding objects in sets.
		typedef std::pair<sibling_iterator, sibling_iterator> range_t;
		typedef std::vector<range_t>                          range_vector_t;

		bool     contains(sibling_iterator from, sibling_iterator to, sibling_iterator arg);
		void     find_argument_lists(range_vector_t&, bool only_comma_lists=true) const;       
		template<class Iter>
		range_vector_t::iterator find_arg_superset(range_vector_t&, Iter st, Iter nd);
		range_vector_t::iterator find_arg_superset(range_vector_t&, sibling_iterator it);

		/// Determine structure (version-2)
		bool     is_termlike(iterator);
		bool     is_factorlike(iterator);
		
		/// Take a single non-product node in a sum and wrap it in a 
		/// product node, so it can be handled on the same footing as a proper product.
		bool     is_single_term(iterator);
		bool     is_nonprod_factor_in_prod(iterator);
		bool     prod_wrap_single_term(iterator&);
		bool     prod_unwrap_single_term(iterator&);

		/// Wrap a term in a product, irrespective of its parent (it usually makes
		/// more sense to call the safer prod_wrap_single_term above).
		void     force_prod_wrap(iterator&);

		/// Figure out whether two objects (commonly indices) are separated by a derivative
		/// operator, as in \partial_{a}{A_{b}} C^{b}.
		/// If the last iterator is pointing to a valid node, check whether it is
		/// independent of the derivative (using the Depends property).
		bool     separated_by_derivative(iterator, iterator, iterator check_dependence) const;

		// Given a node with non-zero multiplier, distribute this
		// multiplier up the tree when the node is a \sum node, or push it into the
		// \prod node if that is the parent. Do this recursively
		// in case a child is a sum as well. Note that 'pushup' is actually 'pushdown'
      // in the case of sums. 
		// This never changes the tree structure, only the distribution of multipliers.
		void     pushup_multiplier(iterator);

		// Return the number of elements in the first range for which an identical element
		// is present in the second range.
		template<class BinaryPredicate>
		unsigned int intersection_number(sibling_iterator, sibling_iterator, 
													sibling_iterator, sibling_iterator, BinaryPredicate) const;

		// Turn a node into a '1' or '0' node.
		void     node_zero(iterator);
		void     node_one(iterator);
		void     node_integer(iterator, int);

		/// A map from a pattern to the position where it occurs in the tree. 
		typedef std::multimap<exptree, exptree::iterator, tree_exact_less_for_indexmap_obj> index_map_t;
		/// A map from the position of each index to the sequential index.
		typedef std::map<exptree::iterator, int, exptree::iterator_base_less>    index_position_map_t;

		/// Index manipulation and classification
		///
		/// Routines to find and classify all indices in an expression, taking into account
		/// sums and products. Note that dummy indices do not always come in pairs, for 
		/// instance in expressions like
		///            a_{m n} ( b^{n p} + q^{n p} ) .
		/// Similarly, free indices can appear multiple times, as in
		///            a_{m} + b_{m} . 
		void     fill_index_position_map(iterator, const index_map_t&, index_position_map_t&) const;
		void     fill_map(index_map_t&, sibling_iterator, sibling_iterator) const;
		void     print_classify_indices(std::ostream&, iterator) const;
		void     determine_intersection(index_map_t& one, index_map_t& two, index_map_t& target,
										  bool move_out=false) const; 
		void     classify_add_index(iterator it, index_map_t& ind_free, index_map_t& ind_dummy) const;
		void     classify_indices_up(iterator, index_map_t& ind_free, index_map_t& ind_dummy) const;
		void     classify_indices(iterator, index_map_t& ind_free, index_map_t& ind_dummy) const;
		int      max_numbered_name_one(const std::string& nm, const index_map_t * one) const;
		int      max_numbered_name(const std::string&, const index_map_t *m1, const index_map_t *m2=0,
											const index_map_t *m3=0, const index_map_t *m4=0, const index_map_t *m5=0) const;
		exptree get_dummy(const list_property *, const index_map_t *m1, const index_map_t *m2=0,
											const index_map_t *m3=0, const index_map_t *m4=0, const index_map_t *m5=0) const;
		exptree get_dummy(const list_property *, iterator) const;
		exptree get_dummy(const list_property *, iterator, iterator) const;

	private:
		// Single or deep-scan apply operations. Do not call directly.
		result_t apply_once(exptree::iterator& it);
		result_t apply_deep(exptree::iterator& it);

		/// Given a node with zero multiplier, propagate this zero
		/// upwards in the tree.  Changes the iterator so that it points
		/// to the next node in a post_order traversal (post_order:
		/// children first, then node). The second node is the topmost
		/// node, beyond which this routine is not allowed to touch the
		/// tree (i.e. the 2nd iterator will always remain valid).
		void     propagate_zeroes(post_order_iterator&, const iterator&);
		void     dumpmap(std::ostream&, const index_map_t&) const;

		bool cleanup_anomalous_products(exptree& tr, exptree::iterator& it);
};


/// Determine the number of elements in the first range which also occur in the
/// second range.
template<class BinaryPredicate>
unsigned int Algorithm::intersection_number(sibling_iterator from1, sibling_iterator to1,
														  sibling_iterator from2, sibling_iterator to2,
														  BinaryPredicate fun) const
	{
	unsigned int ret=0;
	sibling_iterator it1=from1;
	while(it1!=to1) {
		sibling_iterator it2=from2;
		while(it2!=to2) {
			if(fun(*it1,*it2))
				++ret;
			++it2;
			}
		++it1;
		}
	return ret;
	}


