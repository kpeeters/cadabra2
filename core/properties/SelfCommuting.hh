

#pragma once

#include "properties/SelfCommutingBehaviour.hh"

namespace cadabra {

	class SelfCommuting : virtual public SelfCommutingBehaviour {
		public:
			virtual std::string name() const override;
			virtual int sign() const override;
	};

}
