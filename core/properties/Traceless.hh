
#pragma once

#include "Props.hh"

namespace cadabra {

	class Traceless : virtual public property {
		public:
			virtual ~Traceless() {};
			virtual std::string name() const;
		};

	}

