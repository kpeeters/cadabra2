
#pragma once

#include "Props.hh"

class SelfCommutingBehaviour : virtual public property {
	public:
		virtual int sign() const=0;
};
