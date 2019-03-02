
#pragma once

#include "Algorithm.hh"
#include "algorithms/sort_product.hh"

namespace cadabra {

	/// \ingroup algorithms
	///
	/// Generic substitution algorithm.

	class substitute : public Algorithm {
		public:
			substitute(const Kernel&, Ex& tr, Ex& args);

			/// Match the lhs of the replacement rule to the subtree 'st' under consideration.
			/// This will fill the replacement_map giving a map from objects appearing in the
			/// _rule_ to what they matched to.
			///
			/// For F_{m n} -> G_{m n}, position-free indices and the
			/// expression F^{m n}, we would get a replacement map
			///
			///      _m  -> ^m
			///      _n  -> ^n
			///
			/// (Note how it is important that despite these indices being position-free,
			/// the rules in the replacement map do not ignore sub/superscript information).
			/// In the apply member, we then replace all of 'st' with the rhs of the rule,
			/// and scan through the rule (with a basic replacement_map.find based on
			/// simple tree comparison logic, not pattern matching) for replacements to be made.

			virtual bool     can_apply(iterator st);

			virtual result_t apply(iterator&);

			Ex_comparator comparator;
		private:
			Ex&        args;

			iterator      use_rule;
			iterator      conditions;

			std::map<iterator, bool> lhs_contains_dummies, rhs_contains_dummies;

			// For object swap testing routines:
			sort_product    sort_product_;
		};

	}
