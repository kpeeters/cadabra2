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
#include "ProgressMonitor.hh"
#include "IndexClassifier.hh"

#include <map>
#include <fstream>
#include <cstddef>

namespace cadabra {

	/// \ingroup core
	///
	/// Base class for all algorithms, containing generic routines and in
	/// particular the logic for index classification. Also contains static
	/// algorithms acting on Ex objects which require property
	/// information and can therefore not be a member of Ex.
	///
	/// In order to implement a new algorithm, subclass Algorithm and
	/// implement the abstract members Algorithm::can_apply and
	/// Algorithm::apply (see there for further documentation).  The
	/// general logic is that the implementation of
	/// Algorithm::apply(iterator&) is not allowed to make the node pointed
	/// at by the iterator invalid. If the algorithm makes the node vanish,
	/// it should indicate so by setting its multiplier to zero; the
	/// calling logic will then take care of cleaning up the subtree
	/// at the node.
	///
	/// The algorithm is, however, allowed to change the node itself or
	/// replace it with another one, as long as it updates the iterator.

	class Algorithm : public IndexClassifier {
		public:
			/// Initialise the algorithm with a reference to the expression
			/// tree, but do not yet do anything with this tree. Algorithms
			/// are not typically allowed to mess with the settings in the
			/// Kernel, so it is passed const.

			Algorithm(const Kernel&, Ex&);

			virtual ~Algorithm();

			typedef Ex::iterator            iterator;
			typedef Ex::post_order_iterator post_order_iterator;
			typedef Ex::sibling_iterator    sibling_iterator;
			typedef Ex::result_t            result_t;

			bool interrupted;

			/// Provide the algorithm with a ProgressMonitor object on which to register
			/// (nested) progress information, to be reported out-of-band to a client.

			void set_progress_monitor(ProgressMonitor *);

			/// The main entry points for running algorithms, which traverse
			/// the tree post-order ('child before parent'). The 'deep' flag
			/// indicates whether sub-expressions should be acted on
			/// too. The 'repeat' flag indicates whether the algorithm
			/// should be applied until the expression no longer
			/// changes. The 'depth' flag, if not equal to -1, indicates the
			/// depth in the tree where the algorithm should start applying.

			result_t  apply_generic(bool deep=true, bool repeat=false, unsigned int depth=0);
			result_t  apply_generic(iterator&, bool deep, bool repeat, unsigned int depth);

			/// Apply algorithm with alternative traversal: starting from
			/// the top node, traverse the tree pre-order ('parent before
			/// child') and once the algorithm acts at a given node, do not
			/// traverse the subtree below anymore.

			result_t  apply_pre_order(bool repeat=false);

			// Global information
			unsigned int     number_of_calls;
			unsigned int     number_of_modifications;
			bool             suppress_normal_output;
			bool             discard_command_node;

			/// Given an expression top node, check index consistency.
			bool      check_consistency(iterator) const;
			bool      check_index_consistency(iterator) const;
			/// Given an expression top node, check differential form degree consistency.
			bool      check_degree_consistency(iterator) const;

			void report_progress(const std::string&, int todo, int done, int count=2);

			mutable Stopwatch index_sw;
			mutable Stopwatch get_dummy_sw;
			mutable Stopwatch report_progress_stopwatch;

			index_iterator begin_index(iterator it) const;
			index_iterator end_index(iterator it) const;

			// The number of indices of a node, taking into account IndexInherit-ance. These
			// indices do therefore not all have to be direct child nodes of 'it', they can
			// sit deeper down the tree.
			unsigned int number_of_indices(iterator it);
			static unsigned int number_of_indices(const Properties&, iterator it);

			// The number of indices of a node, counting only the direct ones (i.e. not those
			// inherited from child nodes).
			static unsigned int number_of_direct_indices(iterator it);

			// The set to which the first index belongs..
			std::string get_index_set_name(iterator it) const;

			/// Rename the dummies in the sub-tree starting with head at the given iterator.
			/// Ensures that no dummies in this sub-tree overlap with the indices elsewhere
			/// in the tree.			
			bool     rename_replacement_dummies(iterator, bool still_inside_algo=false);

			/// Determines whether the indicated node is 'like a term in a
			/// sum'.  This requires that the node is not a `\sum` node, not
			/// a child of a `\prod` node, and that its parent rel is of
			/// argument-type (p_none).
			static bool     is_termlike(iterator);

			/// Determines whether the indicated node is 'like a factor in a product'.
			/// This requires that the parent is a `\prod' node.
			static bool     is_factorlike(iterator);


		protected:
			Ex& tr;
			ProgressMonitor *pm;

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

			// Locate objects inside the tree (formerly in the 'locate' algorithm).
			unsigned int locate_single_object(Ex::iterator obj_to_find,
			                                  Ex::iterator st, Ex::iterator nd,
			                                  std::vector<unsigned int>& store);
			bool         locate_object_set(const Ex& objs,
			                               Ex::iterator st, Ex::iterator nd,
			                               std::vector<unsigned int>& store);
			static bool  compare_(const str_node&, const str_node&);


			/// Take a single non-product node in a sum and wrap it in a
			/// product node, so it can be handled on the same footing as a proper product.
			bool     is_single_term(iterator);
			bool     is_nonprod_factor_in_prod(iterator);
			bool     prod_wrap_single_term(iterator&);
			bool     prod_unwrap_single_term(iterator&);
			bool     sum_wrap_single_term(iterator&);
			bool     sum_unwrap_single_term(iterator&);

			/// Wrap a term in a product or sum in a node with indicated
			/// name, irrespective of its parent (it usually makes more
			/// sense to call the safer prod_wrap_single_term or
			/// sum_wrap_single_term above). Sets the iterator to the
			/// new node.
			void     force_node_wrap(iterator&, std::string);

			/// Figure out whether two objects (commonly indices) are separated by a derivative
			/// operator, as in \f[ \partial_{a}{A_{b}} C^{b} \f].
			/// If the last iterator is pointing to a valid node, check whether it is
			/// independent of the derivative (using the Depends property).
			bool     separated_by_derivative(iterator, iterator, iterator check_dependence) const;

			// Given a node with non-zero multiplier, distribute this
			// multiplier up the tree when the node is a \sum node, or push it into the
			// `\prod` node if that is the parent. Do this recursively
			// in case a child is a sum as well. Note that 'pushup' is actually 'pushdown'
			// in the case of sums.
			// This never changes the tree structure, only the distribution of multipliers.

			// FIXME: this is now superseded by code in Cleanup.cc, and the generic way
			// to make a tree consistent is to call cleanup_dispatch, not pick-and-match from
			// the various algorithms.
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



		protected:
			bool traverse_ldots;

		private:

			// Single or deep-scan apply operations. Do not call directly.
			result_t apply_once(Ex::iterator& it);
			result_t apply_deep(Ex::iterator& it);

			/// Given a node with zero multiplier, propagate this zero
			/// upwards in the tree.  Changes the iterator so that it points
			/// to the next node in a post_order traversal (post_order:
			/// children first, then node). The second node is the topmost
			/// node, beyond which this routine is not allowed to touch the
			/// tree (i.e. the 2nd iterator will always remain valid).
			void     propagate_zeroes(post_order_iterator&, const iterator&);

			//		bool cleanup_anomalous_products(Ex& tr, Ex::iterator& it);
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


	}
