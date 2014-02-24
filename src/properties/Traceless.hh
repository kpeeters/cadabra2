
#pragma once

#include "Props.hh"

class Traceless : virtual public property  {
	public:
		virtual ~Traceless() {};
		virtual std::string name() const;
};

