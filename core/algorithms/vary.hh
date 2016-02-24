
#pragma once

#include "Algorithm.hh"
#include "algorithms/sort_product.hh"

/// \ingroup algorithms
///
/// Generic variational derivative algorithm. 

class vary : public Algorithm {
	public:
		vary(const Kernel&, Ex& tr, Ex& args);

		virtual bool     can_apply(iterator st) override;
		virtual result_t apply(iterator&) override;

	private:
		Ex&        args;
		
};
