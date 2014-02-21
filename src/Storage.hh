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

/* 
   The storage.cc and props.cc files form one module which does not
   depend on any other file in the system (except of course on the gmp
   library).

*/

#pragma once

#include <iostream>
#include <gmpxx.h>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <stdint.h>
#include <assert.h>

#include "tree.hh"

typedef mpq_class               multiplier_t;
typedef std::set<std::string>   nset_t;
typedef std::set<multiplier_t>  rset_t;
typedef uintptr_t               hashval_t;

long        to_long(multiplier_t);
std::string to_string(long);

extern nset_t name_set;
extern rset_t rat_set;

class str_node { // size: 9 bytes (32 bit arch), can be reduced to 5 bytes.
	public:
		enum bracket_t     { b_round=0, b_square=1, b_curly=2, b_pointy=3, b_none=4, b_no=5, b_invalid=6 };
		enum parent_rel_t  { p_sub=0, p_super=1, p_none=2, p_property=3, p_exponent=4 };

		str_node(void);
		str_node(nset_t::iterator name, bracket_t btype=b_none, parent_rel_t ptype=p_none);
		str_node(const std::string& name, bracket_t btype=b_none, parent_rel_t ptype=p_none);
		
		bool operator==(const str_node&) const;
		bool operator<(const str_node&) const;

		// FIXME: In upcoming gcc, these can no longer be packed. So change these
		// to POD types and save a few bytes as well.
		nset_t::iterator name;
		rset_t::iterator multiplier;

		struct flag_t { // kept inside 8 bits for speed and size
				bool            keep_after_eval : 1; 
				bracket_t       bracket         : 3; 
				parent_rel_t    parent_rel      : 3; 
				bool            line_per_node   : 1;
		}; 

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
		bool is_index() const;            // _ or ^ parent_rel
		bool is_quoted_string() const;
		bool is_command() const;
		bool is_inert_command() const;
		bool is_name_wildcard() const;    //  ?
		bool is_object_wildcard() const;  //  ??
		bool is_range_wildcard() const;   //  #{..}
		bool is_autodeclare_wildcard() const; // m#
		bool is_indexstar_wildcard() const; // ?* in sub/super
		bool is_indexplus_wildcard() const; // ?+ in sub/super
		bool is_numbered_symbol() const;  //  [a-zA-Z]+[0-9]+

		nset_t::iterator name_only();

		static bool compare_names_only(const str_node&, const str_node&);
		static bool compare_name_brack_par(const str_node&, const str_node&);
		static bool compare_name_inverse_par(const str_node&, const str_node&);
}; 

// Helper functions for manipulation of multipliers
void     multiply(rset_t::iterator&, multiplier_t);
void     add(rset_t::iterator&, multiplier_t);
void     zero(rset_t::iterator&);
void     one(rset_t::iterator&);
void     flip_sign(rset_t::iterator&);
void     half(rset_t::iterator&);

class exptree : public tree<str_node> {
	public:
		exptree();
		exptree(tree<str_node>::iterator);
		exptree(const str_node&);

		std::ostream& print_entire_tree(std::ostream& str) const;
		static std::ostream& print_recursive_treeform(std::ostream& str, exptree::iterator it);
		static std::ostream& print_recursive_treeform(std::ostream& str, exptree::iterator it, unsigned int& number);

		// Step up until matching node is found (if current node matches, do nothing)
		iterator     named_parent(iterator it, const std::string&) const;
		iterator     erase_expression(iterator it);
		iterator     keep_only_last(iterator it);

		// Calculate the hash value for the subtree starting at 'it'
		hashval_t    calc_hash(iterator it) const;

		// Quick access to arguments or argument lists for A(B)(C,D) type nodes.
		static sibling_iterator arg(iterator, unsigned int);
		static unsigned int     arg_size(sibling_iterator);

		multiplier_t     arg_to_num(sibling_iterator, unsigned int) const; // shorthand for numerical arguments

		// Like 'child', but using index iterators instead.
//		sibling_iterator tensor_index(const iterator_base& position, unsigned int) const;

		iterator         active_expression(iterator it) const;
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

		/// Replace the object, keeping the original bracket and parent_rel (originally intended
		/// to replace indices only, but now used also for e.g. normal function arguments, as in
		/// \partial_{z}{ A(z) } with a replacement of z).
		iterator         replace_index(iterator position, const iterator& from);

      /// As in replace_index, but moves the index rather than making a copy (so that iterators
		/// pointing to the original remain valid).
		iterator         move_index(iterator position, const iterator& from);

		/// Make sure that the node pointed to is a \comma object, i.e. wrap the node if not already
		/// inside such a \comma.
		void             list_wrap_single_element(iterator&);
		void             list_unwrap_single_element(iterator&);

		/// Replace the node with the only child of the node, useful for e.g.
		/// \prod{A} -> A. This algorithm takes care of the multiplier of the 
		/// top node, i.e. it does 2\prod{A} -> 2 A.
		iterator         flatten_and_erase(iterator position);
};



// Compare two nset iterators by comparing the strings to which they point.

class nset_it_less {
	public:
		bool operator()(nset_t::iterator first, nset_t::iterator second) const;
};

