
#pragma once

#include "properties/IndexInherit.hh"

class Accent : public PropertyInherit, public IndexInherit, virtual public property {
	public:
		virtual std::string name() const;
};

