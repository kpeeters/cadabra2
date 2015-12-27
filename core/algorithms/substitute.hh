
#pragma once

#include "Algorithm.hh"
#include "algorithms/sort_product.hh"

/// \ingroup algorithms
///
/// Generic substitution algorithm. 

class substitute : public Algorithm {
	public:
		substitute(Kernel&, Ex& tr, Ex& args);

		/// Match the lhs of the replacement rule to the subtree 'st' under consideration.
      /// This will fill the replacement_map giving a map from objects appearing in the
		/// _rule_ to what they matched to.
		///
		/// For F_{m n} -> G_{m n} and the expression F^{m n}, we would get a replacement map
		///
      ///      _m  -> ^m
      ///      _n  -> ^n
		///
		/// In the apply member, we then replace all of 'st' with the rhs of the rule, 
		/// and scan through the rule (with a basic replacement_map.find based on
		/// simple tree comparison logic, not pattern matching) for replacements to be made.

		virtual bool     can_apply(iterator st);

		virtual result_t apply(iterator&);
	private:
		Ex&        args;

		iterator      use_rule;
		iterator      conditions;

		Ex_comparator comparator;
		std::map<iterator, bool> lhs_contains_dummies, rhs_contains_dummies;

		// For object swap testing routines:
		sort_product    sort_product_;
};
