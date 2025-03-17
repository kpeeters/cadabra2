
#pragma once

#include "Algorithm.hh"
#include "NTensor.hh"
#include "NEvaluator.hh"
#include <pybind11/stl.h>

namespace cadabra {

	class nval : public Algorithm {
		public:
			nval(const Kernel&, Ex&, NEvaluator&);

			virtual bool     can_apply(iterator) override;
			virtual result_t apply(iterator&) override;

		private:
			const std::vector<std::pair<Ex, NTensor>> values;
			NEvaluator& evaluator;
	};

}
