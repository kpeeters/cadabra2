
#pragma once

#include "Compare.hh"

/// \ingroup algorithms
///
/// Evaluate a tensorial expression to components, performing all
/// dummy index sums.

/**

	See tests/components.cdb for basic samples. 

     ex:= A_{m n} ( A_{m n} + B_{m n} );
     crds:= m -> { t, r, \phi, \theta };
     vals:= { A_{t t} -> r, A_{r r} -> r**2, A_{t \phi} -> t, B_{t t} -> 1 };
     evaluate(ex, crds, vals);
     tst:= r * ( r + 1 ) + r**2 * r**2 + t*t - @(ex);

	The algorithm performs
	the following steps.  First, all free and dummy indices are
	classified using members of Algorithm (maybe later?). For all indices, the 'crds'
	argument to the algorithm is scanned for a

        m -> { a,b,c }

   type pattern. 

   Do a dept-first scan until the first sum or product node. 

     In a sum node, all terms must have the same index structure, so
     we can now make a map from the subtree pattern to all possible
     index values. For each such index value set, lookup if there is a
     replacement pattern, if not replace by zero.  This map can be
     reused later for equal sub-expressions (here it is important that
     a normal tensor expression can be used as a pattern immediately,
     though I would search both on explicit iterator (for things that
     we have just seen) and on pattern (for things that occur again
     later in the tree)).

     In a product node, find all dummies and free indices. Create a 
     set of all index value sets, e.g.

        {m, n, p} ->
         { {t, t, t}, {r,t,t}, {t,r,t}, ... }

     For each such index value set, lookup replacement rules.

     KEY: quick way to lookup, for a given set of index values on a
     (found) pattern/pattern pointed to by iterator, whether and what is the
     value of the pattern.
     

 */

#include "Algorithm.hh"
#include "properties/Indices.hh"

class evaluate : public Algorithm {
	public:
		evaluate(const Kernel&, Ex&, const Ex& component_values);

		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;
		
		/// Merge the information in two 'components' nodes at the given
		/// iterators, moving all out of the second one into the first
		/// one.
		void merge_components(iterator it1, iterator it2);

	private:
		const Ex& components;

		bool is_component(iterator it) const;

		void handle_components(iterator it);
		void handle_sum(iterator it);
		void handle_prod(iterator it);
		void handle_derivative(iterator it);

		/// Replace a single factor with a 'components' ...
		/// The full_ind_free argument can contain a list of indices in the order
		/// in which values should be stored in index value sets.

		void handle_factor(sibling_iterator& sib, const index_map_t& full_ind_free);

		/// Merge entries in a single 'components' node when they are for the
		/// same index value(s).
		void merge_component_children(iterator it);

		/// Cleanup all components in a 'components' node; that is, call the 
		/// cleanup_dispatch function on them.
		void cleanup_components(iterator it1);

		/// Simplify all components of a 'components' node by running sympy's simplify
		/// on them.
		void simplify_components(iterator);

		/// Determine all the Coordinate dependencies of the object at 'it'. For the
		/// time being this can only be a 'components' node.
		std::set<Ex, tree_exact_less_obj> dependencies(iterator it);
}; 
