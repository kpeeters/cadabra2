
#pragma once

#include "properties/CommutingBehaviour.hh"

namespace cadabra {

	class Commuting : virtual public CommutingBehaviour {
		public:
			virtual std::string name() const;
			virtual int sign() const
				{
				return 1;
				}
		};

	}
