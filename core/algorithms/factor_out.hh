
#pragma once

#include "Algorithm.hh"

/// \ingroup algorithms

class factor_out : public Algorithm {
	public:
		factor_out(const Kernel&, Ex&, Ex&);

		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;

	protected:
		typedef std::vector<Ex>    to_factor_out_t;
		to_factor_out_t            to_factor_out;

	private:
		void extract_factors(sibling_iterator product, bool left_to_right, Ex& collector);
		void order_factors(sibling_iterator product, Ex& collector);
		void order_factors(sibling_iterator product, Ex& collector, sibling_iterator first_unordered_term);
};

