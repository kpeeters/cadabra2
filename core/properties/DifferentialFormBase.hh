
#pragma once

#include "Props.hh"
#include "properties/ImplicitIndex.hh"
#include "properties/IndexInherit.hh"

class DifferentialFormBase : virtual public property {
	public:
		virtual Ex degree(const Properties&, Ex::iterator) const = 0;
};
