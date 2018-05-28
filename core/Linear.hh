
#pragma once

#include <vector>
#include "Storage.hh"

namespace linear {
//	bool gaussian_elimination(const std::vector<std::vector<multiplier_t> >&, const std::vector<multiplier_t>& );
	bool gaussian_elimination_inplace(std::vector<std::vector<cadabra::multiplier_t> >&, std::vector<cadabra::multiplier_t>& );
};
