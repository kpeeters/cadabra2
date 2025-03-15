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
			// Initialise with an Ex subtree containing the integrand.
			NIntegrator(Ex::iterator);

			// Set integration range.
			void set_range(const Ex&, double from, double to);
			
			// Entry point.
			std::complex<double> integrate();

			// Object to evaluate the expression. Users of `NIntegrator`
			// can set values for variables in this evaluator.
			NEvaluator evaluator;
			
		private:
			Ex::iterator integrand;
         Ex           ivar;
			double       range_from, range_to;
};
	
};
