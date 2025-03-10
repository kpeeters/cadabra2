#pragma once

#include "Storage.hh"
#include "Compare.hh"
#include "Exceptions.hh"
#include "NTensor.hh"
#include "NEvaluator.hh"

namespace cadabra {

	/// \ingroup numerical
	///
	/// Functionality to numerically integrate definite integrals.
	
	class NIntegrator {
		public:
			// Initialise with an Ex containing the integrand.
			NIntegrator(const Ex&);

			// Set integration range.
			void set_range(const Ex&, double from, double to);
			
			// Entry point.
			std::complex<double> integrate();
			
		private:
			Ex         integrand, ivar;
			NEvaluator evaluator;
			double     range_from, range_to;
};
	
};
