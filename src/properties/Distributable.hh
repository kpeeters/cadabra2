
#pragma once

#include "CoreProps.hh"

class Distributable : virtual public  property {
	public:
		virtual ~Distributable() {};
		virtual std::string name() const;
};

