
#pragma once

#include "properties/Accent.hh"
#include "properties/Distributable.hh"

class DiracBar : public Accent, public Distributable, virtual public property {
	public:
		virtual std::string name() const;
};

