
#pragma once

#include "Props.hh"
#include "properties/Symmetric.hh"

namespace cadabra {

	class Diagonal : public Symmetric {
		public:
			virtual std::string name() const;
		};

	}
