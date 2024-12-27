
#pragma once

#include "Compare.hh"


/// \ingroup algorithms
///
/// Evaluate a tensorial expression to components, performing all
/// dummy index sums.

/**

See tests/components.cdb for basic samples.

Components nodes have the following structure:

  \components_{a b ... n}({t,t,...r}=value, {t,t,...r}=value, ...)


			\verbatim

  			\components
     _{m}            // index names/types
     _{n}
     			\comma          // last child
         			\equals
            			\comma   // list of index values
                	r
                	t
            [value]  // value of tensor for above values of indices
         			\equals
            	...
         			\equals
            	...


  	ex:= A_{m n} ( A_{m n} + B_{m n} );
  	crds:= m -> { t, r, \phi, \theta };
  	vals:= { A_{t t} -> r, A_{r r} -> r**2, A_{t \phi} -> t, B_{t t} -> 1 };
  	evaluate(ex, crds, vals);
  	tst:= r * ( r + 1 ) + r**2 * r**2 + t*t - @(ex);

			\endverbatim

The algorithm performs
	the following steps.  First, all free and dummy indices are
	classified using members of Algorithm (maybe later?). For all indices, the 'crds'
	argument to the algorithm is scanned for a

			\verbatim

     	m -> { a,b,c }

			\endverbatim

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

  			\verbatim

     			{m, n, p} ->
      			{ {t, t, t}, {r,t,t}, {t,r,t}, ... }

  			\endverbatim

  For each such index value set, lookup replacement rules.

  KEY: quick way to lookup, for a given set of index values on a
  (found) pattern/pattern pointed to by iterator, whether and what is the
  	value of the pattern.


*/

#include "Algorithm.hh"
#include "properties/Indices.hh"

namespace cadabra {

	class evaluate : public Algorithm {
		public:
			evaluate(const Kernel&, Ex&, const Ex& component_values, bool rhs=false, bool simplify=true);

			virtual bool     can_apply(iterator) override;
			virtual result_t apply(iterator&) override;

			/// Merge the information in two 'components' nodes at the given
			/// iterators, moving all out of the second one into the first
			/// one.
			void merge_components(iterator it1, iterator it2);

			/// Simplify all components of a 'components' node by
			/// collecting terms and optionally running sympy's simplify
			/// on them. Returns a replacement iterator to the
			/// top. Removes entries for vanishing components.
			void simplify_components(iterator, bool run_sympy=true);

		private:
			const Ex& components;
			bool only_rhs, call_sympy;

			bool is_component(iterator it) const;
			bool is_scalar_function(iterator it) const;

			iterator handle_components(iterator it);
			iterator handle_sum(iterator it);
			iterator handle_prod(iterator it);
			iterator handle_derivative(iterator it);
			iterator handle_epsilon(iterator it);

			/// Replace a single factor with a 'components' ...
			/// The full_ind_free argument can contain a list of indices in the order
			/// in which values should be stored in index value sets.

			iterator handle_factor(sibling_iterator sib, const index_map_t& full_ind_free);

			/// Expand a tensor factor into a components node with all components
			/// written out explicitly. Used when there is no sparse rule matching
			/// this factor.
			iterator dense_factor(iterator sib, const index_map_t& ind_free, const index_map_t& ind_dummy);

			/// Merge entries in a single 'components' node when they are for the
			/// same index value(s).
			void merge_component_children(iterator it);

			/// Cleanup all components in a 'components' node; that is, call the
			/// cleanup_dispatch function on them.
			void cleanup_components(iterator it1);

			/// Wrap a non-component scalar node in a 'components' node.
			iterator wrap_scalar_in_components_node(iterator sib);

			/// Inverse of the above.
			void     unwrap_scalar_in_components_node(iterator sib);

			/// Determine all the Coordinate dependencies of the object at 'it'. For the
			/// time being this can only be a 'components' node.
			std::set<Ex, tree_exact_less_obj> dependencies(iterator it);
		};

	}
