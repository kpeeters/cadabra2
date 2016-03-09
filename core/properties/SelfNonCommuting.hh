
#pragma once

#include "properties/SelfCommutingBehaviour.hh"

class SelfNonCommuting : virtual public SelfCommutingBehaviour {
	public:
		virtual std::string name() const override;
		virtual int sign() const override;
};

