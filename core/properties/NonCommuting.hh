
#pragma once

#include "properties/CommutingBehaviour.hh"

class NonCommuting : virtual public CommutingBehaviour {
	public:
		virtual std::string name() const;
		virtual int sign() const { return 0; }
};

