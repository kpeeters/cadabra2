#pragma once

#include "Algorithm.hh"
#include "properties/GammaMatrix.hh"

namespace cadabra {

class join_gamma : public Algorithm {
	public:
		join_gamma(const Kernel&, Ex&, bool expand, bool use_gendelta);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);

		bool expand;
		std::vector<int>     only_expand;
		const GammaMatrix   *gm1, *gm2;
	private:
		void regroup_indices_(sibling_iterator, sibling_iterator, unsigned int,
									 std::vector<Ex>&, std::vector<Ex>& );
		void append_prod_(const std::vector<Ex>& r1, const std::vector<Ex>& r2, 
								unsigned int num1, unsigned int num2, unsigned int i, multiplier_t mult,
								Ex& rep, iterator loc);

		bool                use_generalised_delta_;
		Ex::iterator   gamma_name_;
		str_node::bracket_t gamma_bracket_;
};

}
