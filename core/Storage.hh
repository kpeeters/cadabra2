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

#include <cstddef>
#include <iostream>
#include <gmpxx.h>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <stdint.h>
#include <assert.h>
#include <initializer_list>

#include "tree.hh"

namespace cadabra {

	typedef mpq_class               multiplier_t;
	typedef std::set<std::string>   nset_t;
	typedef std::set<multiplier_t>  rset_t;
	typedef uintptr_t               hashval_t;

	long        to_long(multiplier_t);
	double      to_double(multiplier_t);
	std::string to_string(long);

	extern nset_t name_set;
	extern rset_t rat_set;

	/// \ingroup core
	///
	/// Elementary building block for a mathematical expression. Contains information about the
	/// way in which the node is related to the parent node, and iterators into the global
	/// list of names and rationals.

	class str_node { // size: 9 bytes (32 bit arch), can be reduced to 5 bytes.
		public:
			enum bracket_t     { b_round=0, b_square=1, b_curly=2, b_pointy=3, b_none=4, b_no=5, b_invalid=6 };

			/// Child nodes are related to their parent node by a so-called parent relation, which can
			/// be one of these values.
			enum parent_rel_t  { p_sub=0, p_super=1, p_none=2, p_property=3, p_exponent=4, p_components=5, p_invalid=7 };

			str_node(void);
			str_node(nset_t::iterator name, bracket_t btype=b_none, parent_rel_t ptype=p_none);
			str_node(const std::string& name, bracket_t btype=b_none, parent_rel_t ptype=p_none);
			str_node(const std::u32string& name, bracket_t btype=b_none, parent_rel_t ptype=p_none);

			bool operator==(const str_node&) const;
			bool operator<(const str_node&) const;

			nset_t::iterator name;
			rset_t::iterator multiplier;

#ifdef _WIN32
			struct flag_t {
				bool            keep_after_eval ;
				bracket_t       bracket         ;
				parent_rel_t    parent_rel      ;
				bool            line_per_node   ;
				};
#else
			struct flag_t { // kept inside 8 bits for speed and size
				bool            keep_after_eval : 1;
				bracket_t       bracket         : 3;
				parent_rel_t    parent_rel      : 3;
				bool            line_per_node   : 1;
				};
#endif

			flag_t fl;

			/// Change the parent relation from sub to super and vice versa (throws error
			/// when this is not an index).
			void flip_parent_rel();

			bool is_zero() const;
			bool is_identity() const;
			bool is_rational() const;
			bool is_unsimplified_rational() const;
			bool is_integer() const;
			bool is_unsimplified_integer() const;
			bool is_index() const;            // _ or ^ parent_rel (does not query properties)
			bool is_quoted_string() const;
			bool is_command() const;
			bool is_inert_command() const;
			bool is_name_wildcard() const;    //  ?
			bool is_object_wildcard() const;  //  ??
			bool is_range_wildcard() const;   //  #{..}
			bool is_siblings_wildcard() const; // a...
			bool is_autodeclare_wildcard() const; // m#
			bool is_indexstar_wildcard() const; // ?* in sub/super
			bool is_indexplus_wildcard() const; // ?+ in sub/super
			bool is_numbered_symbol() const;  //  [a-zA-Z]+[0-9]+

			nset_t::iterator name_only();

			static bool compare_names_only(const str_node&, const str_node&);
			static bool compare_name_brack_par(const str_node&, const str_node&);
			static bool compare_name_inverse_par(const str_node&, const str_node&);
		};

	/// \ingroup core
	///
	/// Helper functions for manipulation of multipliers.
	void     multiply(rset_t::iterator&, multiplier_t);
	void     add(rset_t::iterator&, multiplier_t);
	void     zero(rset_t::iterator&);
	void     one(rset_t::iterator&);
	void     flip_sign(rset_t::iterator&);
	void     half(rset_t::iterator&);
	void     set(rset_t::iterator&, multiplier_t);
	
	/// \ingroup core
	///
	/// Basic storage class for symbolic mathemematical expressions. The
	/// full meaning of an expression typically requires knowledge about
	/// properties of patterns in it, which this class does _not_
	/// contain. All property dependent algorithms acting on Ex
	/// objects are in Algorithm.hh.

	class Ex : public std::enable_shared_from_this<Ex>, public tree<str_node> {
		public:
			Ex();
			//		Ex(const tree<str_node>&);
			Ex(tree<str_node>::iterator);
			Ex(const str_node&);
			Ex(const Ex&);
			/// Initialise with given string as head node (does not parse this string).
			Ex(const std::string&);
			Ex(int);

			/// Keeping track of what algorithms have done to this expression.
			/// After a reset_state (or at initialisation), the expression sits
			/// in the 'checkpointed' state. When an algorithm acts, it can then
			/// move to 'no_action' (unchanged), 'applied' (changed) or 'error'.
			/// Once it is in 'error', it will stay there until the next 'reset'.
			/// FIXME: the following should implement a stack of states,
			/// so that it can be used with nested functions.

			enum result_t { l_checkpointed, l_no_action, l_applied, l_applied_no_new_dummies, l_error };
			result_t state() const;
			void     update_state(result_t);
			void     reset_state();

			/// A status query method mainly to implement a simple method to
			/// apply algorithms until they converge. Returns true when the
			/// expression is in 'checkpointed' or 'applied' state. Will
			/// set the state to 'no_action'.
			bool     changed_state();

			/// Test if the expression is a rational number.
			/// FIXME: add tests for integers as well.
			bool          is_rational() const;
			multiplier_t  to_rational() const;
			bool          is_integer() const;
			long          to_integer() const;

			/// Test if the expression is empty (no content at all).
			bool          is_empty() const;

			/// Display expression in Python/Cadabra input form. This is
			/// fairly straightforward so not handled with a separate
			/// DisplayBase derived class.
			static std::ostream& print_python(std::ostream& str, Ex::iterator it);

			/// Output helpers mainly for debugging purposes.
			std::ostream& print_entire_tree(std::ostream& str) const;
			static std::ostream& print_recursive_treeform(std::ostream& str, Ex::iterator it);
			static std::ostream& print_recursive_treeform(std::ostream& str, Ex::iterator it, unsigned int& number);

			/// Print a representation like Python's 'repr'.
			std::ostream& print_repr(std::ostream& str, Ex::iterator it) const;

			/// Step up until matching node is found (if current node matches, do nothing)
			iterator     named_parent(iterator it, const std::string&) const;
			iterator     erase_expression(iterator it);

			/// Calculate the hash value for the subtree starting at 'it'.
			hashval_t    calc_hash(iterator it) const;

			/// Quick access to arguments or argument lists for A(B)(C,D) type nodes.
			static sibling_iterator arg(iterator, unsigned int);
			static unsigned int     arg_size(sibling_iterator);

			multiplier_t     arg_to_num(sibling_iterator, unsigned int) const; // shorthand for numerical arguments

			// Like 'child', but using index iterators instead.
			//		sibling_iterator tensor_index(const iterator_base& position, unsigned int) const;

			// Number of \\history nodes in an expression
			unsigned int     number_of_steps(iterator it) const;
			// Given an iterator pointing to any node in the tree, figure
			// out to which equation number it belongs.
			unsigned int     number_of_equations() const;
			unsigned int     equation_number(iterator it) const;
			nset_t::iterator equation_label(iterator it) const;
			iterator         equation_by_number(unsigned int i) const;
			iterator         equation_by_name(nset_t::iterator it) const;
			iterator         equation_by_name(nset_t::iterator it, unsigned int& ) const;
			iterator         equation_by_number_or_name(iterator it, unsigned int last_used_equation) const;
			iterator         equation_by_number_or_name(iterator it, unsigned int last_used_equation,
			      unsigned int&) const;
			std::string      equation_number_or_name(iterator it, unsigned int last_used_equation) const;
			iterator         procedure_by_name(nset_t::iterator it) const;

			// Determine whether a node has an '\ldots' parent (not necessarily direct).
			bool is_hidden(iterator) const;

			/// Replace the index-like object (originally intended to
			/// replace indices only, but now used also for e.g. normal
			/// function arguments, as in \f[ \partial_{z}{ A(z) } \f] with
			/// a replacement of z).
			///
			/// Note: this originally kept the bracket and parent_rel, but
			/// that is not a good idea, because it prevents us from
			/// changing those. If we want to use a _{z} pattern replacing a
			/// A(z) index, it is better to make a rule that matches (z) and
			/// at the time we find and match _{z}. So this should be
			/// handled by the replacement_map logic in Compare.cc.
			iterator         replace_index(iterator position, const iterator& from, bool keep_parent_rel=false);

			/// As in replace_index, but moves the index rather than making a copy (so that iterators
			/// pointing to the original remain valid).
			iterator         move_index(iterator position, const iterator& from);

			/// Make sure that the node pointed to is a \\comma object, i.e. wrap the node if not already
			/// inside such a \\comma.
			/// DEPRECATED: in favour of 'do_list' in Functional.hh.
			void             list_wrap_single_element(iterator&);
			void             list_unwrap_single_element(iterator&);

			/// Replace the node with the children of the node, useful for e.g.
			/// \\prod{A} -> A. This algorithm takes care of the multiplier of the
			/// top node, i.e. it does 2\\prod{A} -> 2 A. Returns an iterator
			/// to the new location of the first child of the original node.
			iterator         flatten_and_erase(iterator position);

			/// Compare two Ex objects for exact equality; no dummy equivalence or other
			/// things that require property information.

			bool operator==(const Ex& other) const;

			/// Push a copy of the current state of the expression onto the
			/// history stack.  Also pushes a set of paths to terms which
			/// will be kept in the next history step.
			/// DEPRECATED, only used by take_match/replace_match.
			void push_history(const std::vector<Ex::path_t>&);

			/// Pop the most recent state of the expression off the history stack; returns
			/// the set of paths that we are replacing.
			/// DEPRECATED, only used by take_match/replace_match.
			std::vector<Ex::path_t> pop_history();

			/// Return the size of the history; 0 means no history, just the current
			/// expression.
			int  history_size() const;

		private:
			result_t state_;

			std::vector<tree<str_node> > history;
			/// Patterns which describe how to get from one history step to the next.
			std::vector<std::vector<Ex::path_t> > terms;
		};


	/// \ingroup core
	///
	/// Compare two nset iterators by comparing the strings to which they point.

	class nset_it_less {
		public:
			bool operator()(nset_t::iterator first, nset_t::iterator second) const;
		};

	template <typename T>
	bool is_in(const T& val, const std::initializer_list<T>& list)
		{
		for (const auto& i : list) {
			if (val == i) {
				return true;
				}
			}
		return false;
		}

	}

/// \ingroup core
///
/// Bare output operator for Ex objects, mainly to provide a simple
/// way to generate debugging output. Does not do any fancy
/// formatting; just prints a nested list representation.  For more
/// fancy output, look at DisplayTeX, DisplaySympy and
/// DisplayTerminal.

std::ostream& operator<<(std::ostream&, const cadabra::Ex&);
std::ostream& operator<<(std::ostream&, cadabra::Ex::iterator);
