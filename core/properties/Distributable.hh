
#pragma once

#include "Props.hh"

class Distributable : virtual public  property {
	public:
		virtual ~Distributable() {};
		virtual std::string name() const;
};

