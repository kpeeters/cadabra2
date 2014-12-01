
#pragma once

#include "properties/SelfCommutingBehaviour.hh"

class SelfAntiCommuting : virtual public SelfCommutingBehaviour {
	public:
		virtual std::string name() const;
		virtual int sign() const override;
};
