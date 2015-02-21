
#pragma once

#include "Props.hh"
#include "properties/Symmetric.hh"

class Diagonal : public Symmetric {
	public:
		virtual std::string name() const;
};
