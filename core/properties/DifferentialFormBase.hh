
#pragma once

#include "Props.hh"
#include "properties/ImplicitIndex.hh"
#include "properties/IndexInherit.hh"

namespace cadabra {

	class DifferentialFormBase : virtual public property {
		public:
			virtual Ex degree(const Properties&, Ex::iterator) const = 0;
	};

}
