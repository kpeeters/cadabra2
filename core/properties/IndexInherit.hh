
#pragma once

#include "Props.hh"

class IndexInherit : virtual public property {
	public: 
		virtual std::string name() const { return std::string("IndexInherit"); };
};
