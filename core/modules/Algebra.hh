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

#include <string>
#include <map>

#include "Algorithm.hh"
#include "Combinatorics.hh"
#include "Props.hh"
#include "CoreProps.hh"
#include "YoungTab.hh"
#include "Numerical.hh"

#include "properties/AntiSymmetric.hh"
#include "properties/Matrix.hh"



class WeightInherit;
class Weight;


/// General algebraic manipulations and index permutation symmetries
namespace algebra {
	void register_properties();
	void register_algorithms();
};


class Commuting : virtual public CommutingBehaviour {
	public:
		virtual std::string name() const;
		virtual int sign() const { return 1; }
};

class AntiCommuting : virtual public CommutingBehaviour {
	public:
		virtual std::string name() const;
		virtual int sign() const { return -1; }
};

class NonCommuting : virtual public CommutingBehaviour {
	public:
		virtual std::string name() const;
		virtual int sign() const { return 0; }
};

class SelfCommuting : virtual public SelfCommutingBehaviour {
	public:
		virtual std::string name() const;
		virtual int sign() const { return 1; }
};

class SelfAntiCommuting : virtual public SelfCommutingBehaviour {
	public:
		virtual std::string name() const;
		virtual int sign() const { return -1; }
};

class SelfNonCommuting : virtual public SelfCommutingBehaviour {
	public:
		virtual std::string name() const;
		virtual int sign() const { return 0; }
};

class TableauSymmetry : public TableauBase, virtual public property {
	public:
		virtual ~TableauSymmetry() {};
		virtual bool parse(exptree&, exptree::iterator, exptree::iterator, keyval_t&);
		virtual std::string name() const;
		virtual void display(std::ostream&) const;

		virtual unsigned int size(exptree&, exptree::iterator) const;
		virtual tab_t        get_tab(exptree&, exptree::iterator, unsigned int) const;

		virtual bool         only_column_exchange() const { return only_col_; };

		std::vector<tab_t>     tabs;
	private:
		bool only_col_;
};

class SatisfiesBianchi : public TableauBase, virtual public property {
	public:
		virtual bool parse(exptree&, exptree::iterator, exptree::iterator, keyval_t&);
		virtual std::string name() const;

		virtual unsigned int size(exptree&, exptree::iterator) const;
		virtual tab_t        get_tab(exptree&, exptree::iterator, unsigned int) const;
};

class Symmetric : public TableauBase, virtual public property  {
	public:
		virtual ~Symmetric() {};
		virtual bool parse(exptree&, exptree::iterator, exptree::iterator, keyval_t&);
		virtual std::string name() const;

		virtual unsigned int size(exptree&, exptree::iterator) const;
		virtual tab_t        get_tab(exptree&, exptree::iterator, unsigned int) const;
};

class Diagonal : public Symmetric {
	public:
		virtual std::string name() const;
};

// class TracelessBase : virtual public property  {
// 	public:
// 		virtual ~TracelessBase() {};
// 		virtual std::string name() const;
// 
// 		typedef index_pair_t std::pair<unsigned int, unsigned int
// 		virtual std::vector<
// };

class SelfDual : public AntiSymmetric {
	public:
		virtual std::string name() const;
		virtual tab_t        get_tab(exptree&, exptree::iterator, unsigned int) const;
};

class AntiSelfDual : public AntiSymmetric {
	public:
		virtual std::string name() const;
		virtual tab_t        get_tab(exptree&, exptree::iterator, unsigned int) const;
};

class DAntiSymmetric : public TableauBase, virtual public property  {
	public:
		virtual ~DAntiSymmetric() {};
		virtual bool parse(exptree&, exptree::iterator, exptree::iterator, keyval_t&);
		virtual std::string name() const;

		virtual unsigned int size(exptree&, exptree::iterator) const;
		virtual tab_t        get_tab(exptree&, exptree::iterator, unsigned int) const;
};

class KroneckerDelta : public TableauBase, virtual public property {
	public:
		virtual ~KroneckerDelta() {};
		virtual std::string name() const;

		virtual unsigned int size(exptree&, exptree::iterator) const;
		virtual tab_t        get_tab(exptree&, exptree::iterator, unsigned int) const;
};

class EpsilonTensor : public AntiSymmetric, virtual public property {
	public:
		virtual ~EpsilonTensor() {};
		virtual bool parse(exptree&, exptree::iterator, exptree::iterator, keyval_t&);
		virtual std::string name() const;

		exptree metric, krdelta;
};

class prodrule : public algorithm {
	public:
		prodrule(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);

		sibling_iterator prodnode;
		unsigned int     number_of_indices;
};

class remove_indexbracket : public algorithm {
	public:
		remove_indexbracket(exptree&, iterator);
		
		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
};

class distribute : public algorithm {
	public:
		distribute(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
};

class sumsort : public algorithm {
	public:
		sumsort(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);

		bool should_swap(iterator obj, int subtree_comparison) const;
};

class prodsort : public algorithm {
	public:
		prodsort(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);

	private:
		bool ignore_numbers_;
};

class keep_terms : public algorithm {
	public:
		keep_terms(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
};

class reduce_sub : public algorithm {
	public:
		reduce_sub(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
};

class index_object_cleanup : public algorithm {
	public:
		index_object_cleanup(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
};

class prodflatten : public algorithm {
	public:
		prodflatten(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);

		bool make_consistent_only, is_diff;
};

class sumflatten : public algorithm {
	public:
		sumflatten(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);

		bool make_consistent_only;
};

class listflatten : public algorithm {
	public:
		listflatten(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
};

// Move all numerical factors inside a generalised \frac into
// the multiplier field. 

class reduce_div : public algorithm {
	public:
		reduce_div(exptree&, iterator);
		
		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
};

class subseq : public algorithm {
	public:
		subseq(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
};

class collect_factors : public algorithm {
	public:
		collect_factors(exptree&, iterator);
		
		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
	private:
		void fill_hash_map(iterator);

		typedef std::multimap<hashval_t, sibling_iterator> factor_hash_t;
		typedef factor_hash_t::iterator                    factor_hash_iterator_t;

		factor_hash_t factor_hash;
};

class collect_terms : public algorithm {
	public:
		collect_terms(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
//		virtual result_t apply(sibling_iterator&, sibling_iterator&);

		void  fill_hash_map(iterator);
		void  fill_hash_map(sibling_iterator, sibling_iterator);
	private:
		result_t collect_from_hash_map();
		void remove_zeroed_terms(sibling_iterator, sibling_iterator);

		typedef std::multimap<hashval_t, sibling_iterator> term_hash_t;
		typedef term_hash_t::iterator                      term_hash_iterator_t;

		term_hash_t term_hash;
};

class factor_in : public algorithm {
	public:
		factor_in(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);

	protected:
		std::set<exptree, tree_less_obj> factnodes; // objects to be taken in brackets;  
                                                          // FIXME: use patterns
		bool      compare_restricted(iterator one, iterator two) const;
		bool      compare_prod_nonprod(iterator prod, iterator nonprod) const;

		// Calculate the hash value excluding factors given in the argument list
		hashval_t calc_restricted_hash(iterator it) const;
		void      fill_hash_map(iterator);

		typedef std::multimap<hashval_t, sibling_iterator> term_hash_t;
		typedef term_hash_t::iterator                      term_hash_iterator_t;

		term_hash_t term_hash;
};

class factor_out : public algorithm {
	public:
		factor_out(exptree&, iterator);
		
		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);

	protected:
		typedef std::vector<exptree> to_factor_out_t;
		to_factor_out_t             to_factor_out;

	private:
		void extract_factors(sibling_iterator product, bool left_to_right, exptree& collector);
		void order_factors(sibling_iterator product, exptree& collector);
		void order_factors(sibling_iterator product, exptree& collector, sibling_iterator first_unordered_term);
};

class canonicalise : public algorithm {
	public:
		canonicalise(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);		

		std::vector<std::vector<int> > generating_set;
		bool             reuse_generating_set;

	private:
		bool remove_traceless_traces(iterator&);
		bool remove_vanishing_numericals(iterator&);
		bool only_one_on_derivative(iterator index1, iterator index2) const;

		std::string get_index_set_name(iterator) const;
		Indices::position_t  position_type(iterator) const;
//		void collect_dummy_info(const index_map_t&, const index_position_map_t&, 
//										std::vector<int>&, std::vector<int>&);
};

class reduce : public algorithm {
	public:
		reduce(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);		
};


class drop : public algorithm {
	public:
		drop(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);		
};

class drop_keep_weight : public algorithm {
	public:
		drop_keep_weight(exptree&, iterator);
		virtual bool     can_apply(iterator);
		result_t do_apply(iterator&, bool keepthem);
	protected:
		const WeightInherit *gmn;
		const Weight        *wgh;
		std::string label;
		multiplier_t weight;
};

class drop_weight : public drop_keep_weight {
	public:
		drop_weight(exptree&, iterator);

		virtual result_t apply(iterator&);		
};

class keep_weight : public drop_keep_weight {
	public:
		keep_weight(exptree&, iterator);

		virtual result_t apply(iterator&);		
};

class ratrewrite : public algorithm {
	public:
		ratrewrite(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);		
};

class locate : virtual public algorithm {
	public:
		locate(exptree&, iterator);
	protected:
		unsigned int locate_single_object_(exptree::iterator obj_to_find, 
													  exptree::iterator st, exptree::iterator nd,
													  std::vector<unsigned int>& store);
		bool locate_object_set_(exptree::iterator set_parent,
										exptree::iterator st, exptree::iterator nd,
										std::vector<unsigned int>& store);
		bool        locate_(sibling_iterator st, sibling_iterator nd, std::vector<unsigned int>&);
		static bool compare_(const str_node&, const str_node&);
};

class sym_asym : virtual public algorithm, virtual public locate {
	public:
		sym_asym(exptree&, iterator);

		virtual bool     can_apply(iterator);
//		virtual bool     can_apply(sibling_iterator, sibling_iterator);
	protected:
		result_t doit(sibling_iterator&, sibling_iterator&, bool sign);
		std::vector<unsigned int>  argloc_2_treeloc;
		combin::combinations<unsigned int> raw_ints;
};

class sym : virtual public algorithm, virtual public sym_asym {
	public:
		sym(exptree&, iterator);
		
		virtual result_t apply(iterator& it);
//		virtual result_t apply(sibling_iterator&, sibling_iterator&);		
};

class asym : virtual public algorithm, virtual public sym_asym {
	public:
		asym(exptree&, iterator);
		
		virtual result_t apply(iterator& it);
//		virtual result_t apply(sibling_iterator&, sibling_iterator&);
};

class order : virtual public algorithm, virtual public locate {
	public:
		order(exptree&, iterator);

		virtual bool     can_apply(iterator);
	protected:
		result_t doit(iterator&, bool sign);
};

class canonicalorder : virtual public algorithm, virtual public order {
	public:
		canonicalorder(exptree&, iterator);
		
		virtual result_t apply(iterator&);
};

class acanonicalorder : virtual public algorithm, virtual public order {
	public:
		acanonicalorder(exptree&, iterator);
		
		virtual result_t apply(iterator&);
};

class impose_asym : public algorithm {
	public:
		impose_asym(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);		
};

class eqn : public algorithm {
	public:
		eqn(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);		
};

class indexsort : public algorithm {
	public:
		indexsort(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);		

		class less_indexed_treenode {
			public:
				less_indexed_treenode(exptree&, iterator it);
				bool operator()(unsigned int, unsigned int) const;
			private:
				exptree&          tr;  
				exptree::iterator it;
		};
	private:
		const TableauBase     *tb;
};

class asymprop : public algorithm {
	public:
		asymprop(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);		
	private:
		const TableauBase     *tb;		
};

class young_project : public algorithm {
	public:
		young_project(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);		

		// Boxes represent tensor slots, the values in them
		// give the location in the tensor.
		typedef yngtab::filled_tableau<unsigned int> pos_tab_t;
		typedef yngtab::filled_tableau<iterator>     name_tab_t;
		pos_tab_t  tab;
		name_tab_t nametab;
		combin::symmetriser<unsigned int> sym;

		// Implicit antisymmetry; used by tableaux.cc only so far. These
		// store index locations, so these are "values" from the 
		// combinatorics.hh point of view.
		combin::combinations<unsigned int>::permuted_sets_t asym_ranges;
		bool remove_traces;
	private:
		iterator nth_index_node(iterator, unsigned int);
};

class young_project_tensor : public algorithm {
	public:
		young_project_tensor(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);		

		bool modulo_monoterm;
		combin::symmetriser<unsigned int> sym;
	private:
		const TableauBase     *tb;
};

class expand_power : public algorithm {
	public:
		expand_power(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);		
};


