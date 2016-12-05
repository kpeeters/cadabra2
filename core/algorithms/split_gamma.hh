
#pragma once

#include "Algorithm.hh"
#include "properties/GammaMatrix.hh"

class split_gamma : public Algorithm {
	public:
		split_gamma(const Kernel&, Ex&, bool on_back);

		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;

		bool on_back;
};
