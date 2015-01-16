#pragma once

#include "Algorithm.hh"
#include "properties/GammaMatrix.hh"

class join_gamma : public Algorithm {
	public:
		join_gamma(Kernel&, exptree&, bool expand, bool use_gendelta);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);

		bool expand;
		std::vector<int>     only_expand;
		const GammaMatrix   *gm1, *gm2;
	private:
		void regroup_indices_(sibling_iterator, sibling_iterator, unsigned int,
									 std::vector<exptree>&, std::vector<exptree>& );
		void append_prod_(const std::vector<exptree>& r1, const std::vector<exptree>& r2, 
								unsigned int num1, unsigned int num2, unsigned int i, multiplier_t mult,
								exptree& rep, iterator loc);

		bool                use_generalised_delta_;
		exptree::iterator   gamma_name_;
		str_node::bracket_t gamma_bracket_;
};
