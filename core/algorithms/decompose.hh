
#pragma once

#include "Algorithm.hh"

namespace cadabra {

class decompose : public Algorithm {
	public:
      decompose(const Kernel&, Ex&, Ex&);

		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;

   private:
      Ex basis;

		void add_element_to_basis(Ex&, Ex::iterator);
		std::vector<Ex>                    terms_from_yp;
		std::vector<std::vector<multiplier_t> > coefficient_matrix;
      
};

}
