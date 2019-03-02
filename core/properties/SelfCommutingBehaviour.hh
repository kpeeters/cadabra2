
#pragma once

#include "Props.hh"

namespace cadabra {

	class SelfCommutingBehaviour : virtual public property {
		public:
			virtual int sign() const=0;
		};

	}
