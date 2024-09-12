
#pragma once

#include "properties/SelfCommutingBehaviour.hh"

namespace cadabra {

	class SelfAntiCommuting : virtual public SelfCommutingBehaviour {
		public:
			virtual ~SelfAntiCommuting();
			virtual std::string name() const override;
			virtual int sign() const override;
		};

	}
