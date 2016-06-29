
#pragma once

#include "Algorithm.hh"

/// \ingroup algorithms

class einsteinify : public Algorithm {
	public:
		einsteinify(const Kernel&, Ex&, Ex&);

		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;
	private:
		Ex metric;
};

