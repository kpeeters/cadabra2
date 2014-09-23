
#pragma once

#include "Props.hh"

/// Property indicating that an operator is numerically flat, so that
/// numerical factors in the argument can be taken outside.

class NumericalFlat : virtual public property {
	public:
		virtual std::string name() const;
};
