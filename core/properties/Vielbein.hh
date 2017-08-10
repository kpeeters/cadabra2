
#pragma once

#include "Props.hh"

namespace cadabra {

	class Vielbein : virtual public property {
		public:
			virtual std::string name() const;
	};
	
	class InverseVielbein : virtual public property {
		public:
			virtual std::string name() const;
	};

}

