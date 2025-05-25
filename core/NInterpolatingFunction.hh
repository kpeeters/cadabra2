#pragma once

#include "NTensor.hh"
#include "Storage.hh"
#include "Compare.hh"
#include <vector>

namespace cadabra {

	/// An object representing a one-variable function (for now), which
	/// is computed from an interpolation of numerical data points.
	/// FIXME: generalise to functions of more than one variable.
	
	class NInterpolatingFunction {
		public:
			NInterpolatingFunction();

			// FIXME: use NTensor as argument?
			std::complex<double> evaluate(double) const;
			
 			Ex          var;
			NTensor     var_values;
			NTensor     fun_values;

			// Return the range over which the function is
			// defined/computable without extrapolation.
			std::pair<double, double> range() const;

		private:
			mutable NTensor slope_values;
			mutable size_t  last_index;
			mutable bool    precomputed;

			size_t find_interval(double) const;
			void   compute_slopes() const;
	};

	/// Data structure which, for a set of variables/expressions,
	/// holds a real range.
	typedef std::map<Ex, std::pair<double, double>, tree_exact_less_no_wildcards_obj> variable_ranges_t;
	
	/// Return the range of definition of a given expression.
	variable_ranges_t function_domain(const Ex&);

};
