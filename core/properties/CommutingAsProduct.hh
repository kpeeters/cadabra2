
#pragma once

#include "Props.hh"

namespace cadabra {

	class CommutingAsProduct : virtual public property {
		public:
			virtual std::string name() const;
	};

}
