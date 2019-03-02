
#pragma once

#include "Props.hh"

namespace cadabra {

	class IndexInherit : virtual public property {
		public:
			virtual std::string name() const
				{
				return std::string("IndexInherit");
				};
		};

	}
