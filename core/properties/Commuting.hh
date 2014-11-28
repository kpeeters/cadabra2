
#pragma once

#include "properties/CommutingBehaviour.hh"

class Commuting : virtual public CommutingBehaviour {
	public:
		virtual std::string name() const;
		virtual int sign() const { return 1; }
};

