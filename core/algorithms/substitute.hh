
#pragma once

#include "Algorithm.hh"
#include "algorithms/sort_product.hh"

class substitute : public Algorithm {
	public:
		substitute(Kernel&, Ex& tr, Ex& args);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
	private:
		Ex&        args;

		unsigned int    use_rule;

		iterator        conditions;

		Ex_comparator comparator;
		std::vector<bool>  lhs_contains_dummies, rhs_contains_dummies;

		// For object swap testing routines:
		sort_product    sort_product_;
};
