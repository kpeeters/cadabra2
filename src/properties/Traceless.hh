
#pragma once

#include "CoreProps.hh"

class Traceless : virtual public property  {
	public:
		virtual ~Traceless() {};
		virtual std::string name() const;
};

