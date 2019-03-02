
#pragma once

#include "Props.hh"

namespace cadabra {

	class WeightBase : virtual public labelled_property {
		public:
			virtual multiplier_t  value(const Kernel&, Ex::iterator, const std::string& forcedlabel) const=0;
		};

	}
