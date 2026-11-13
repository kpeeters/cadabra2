
#pragma once

#include "Algorithm.hh"

namespace cadabra {

	/// \ingroup algorithms
	///
	/// General purpose brute force scalar simplification algorithm.
	/// Can be switched to use different scalar backends and thus
	/// acts as a uniform front-end for different scalar CAS simplify
	/// algorithms.
	///
	/// By default this calls the backend's generic simplification
	/// ('simplify' for sympy, 'FullSimplify' for Mathematica). A
	/// non-empty 'function' names a backend function to call instead
	/// (e.g. "cancel" or "factor" for sympy, "Together" for Mathematica).

	class simplify : public Algorithm {
		public:
			simplify(const Kernel&, Ex&, const std::string& function="");

			virtual bool     can_apply(iterator);
			virtual result_t apply(iterator&);

		private:
			std::string               function_;
			std::vector<Ex::iterator> left;
			std::set<Ex::iterator>    index_factors;
		};

	}
