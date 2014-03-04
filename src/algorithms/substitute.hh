
#pragma once

#include "Algorithm.hh"
#include "algorithms/sort_product.hh"

class substitute : public Algorithm {
	public:
		substitute(Kernel&, exptree& tr, exptree& args);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
	private:
		exptree&        args;

		unsigned int    use_rule;

		iterator        conditions;

		exptree_comparator comparator;
		std::vector<bool>  lhs_contains_dummies, rhs_contains_dummies;

		// For object swap testing routines:
		sort_product    sort_product_;
};
