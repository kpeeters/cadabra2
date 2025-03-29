#pragma once

#include "NTensor.hh"
#include "Storage.hh"
#include <vector>

namespace cadabra {

	class NInterpolatingFunction {
		public:
			NInterpolatingFunction();

			// FIXME: use NTensor as argument?
			std::complex<double> evaluate(double) const;
			
 			Ex          var;
			NTensor     var_values;
			NTensor     fun_values;

		private:
			mutable NTensor slope_values;
			mutable size_t  last_index;
			mutable bool    precomputed;

			size_t find_interval(double) const;
			void   compute_slopes() const;
	};

};
