
#pragma once

#include "Algorithm.hh"

namespace cadabra {

/// \ingroup algorithms
///
/// Functionality to act with Sympy on all scalar parts of an expression, and
/// keep the result in-place. This is a higher-level thing than
/// 'sympy::apply' in the SympyCdb.cc module.	

class map_sympy : public Algorithm {
	public:
		map_sympy(const Kernel&, Ex&, const std::string& head, std::vector<std::string> args);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);

	private:
		std::string               head_;
		std::vector<std::string>  args_;
		std::vector<Ex::iterator> left;
		std::set<Ex::iterator>    index_factors;
};

}

