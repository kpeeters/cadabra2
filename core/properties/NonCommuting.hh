
#pragma once

#include "properties/CommutingBehaviour.hh"

namespace cadabra {

	class NonCommuting : virtual public CommutingBehaviour {
		public:
			virtual ~NonCommuting();
			virtual std::string name() const;
			virtual int sign() const
				{
				return 0;
				}
		};

	}
