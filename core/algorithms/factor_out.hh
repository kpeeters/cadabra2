
#pragma once

#include "Algorithm.hh"

/// \ingroup algorithms

class factor_out : public Algorithm {
	public:
		factor_out(const Kernel&, Ex&, Ex&);

		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;

	private:
		typedef std::vector<Ex>    to_factor_out_t;
		to_factor_out_t            to_factor_out;
};

