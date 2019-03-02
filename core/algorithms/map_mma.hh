
#pragma once

#include "Algorithm.hh"

namespace cadabra {

	/// \ingroup algorithms
	///
	/// Functionality to act with Mathematica on all scalar parts of an
	/// expression, and keep the result in-place. This is a higher-level
	/// thing than 'mma::apply' in the MMACdb.cc module.

	class map_mma : public Algorithm {
		public:
			map_mma(const Kernel&, Ex&, const std::string& head);

			virtual bool     can_apply(iterator);
			virtual result_t apply(iterator&);

		private:
			std::string               head_;
			std::vector<Ex::iterator> left;
			std::set<Ex::iterator>    index_factors;
		};

	}

