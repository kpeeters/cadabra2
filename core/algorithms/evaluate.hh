
#pragma once

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

     KEY: quick way to lookup, for a given set of index values on a pattern,
     whether and what is the value of the pattern.
     

 */
