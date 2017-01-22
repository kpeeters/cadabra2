
#pragma once

#include "Props.hh"

namespace cadabra {

	class Distributable : virtual public  property {
		public:
			virtual ~Distributable() {};
			virtual std::string name() const;
	};

}
