
#pragma once

#include "Algorithm.hh"

namespace cadabra {

/// \ingroup algorithms
///
/// General purpose brute force scalar simplification algorithm.
/// Can be switched to use different scalar backends and thus
/// acts as a uniform front-end for different scalar CAS simplify
/// algorithms.

class simplify : public Algorithm {
	public:
		simplify(const Kernel&, Ex&);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);

	private:
		enum class backend_t { sympy, mathematica } backend;
		
		std::vector<Ex::iterator> left;
		std::set<Ex::iterator>    index_factors;
};

}
