
#pragma once

#include "properties/IndexInherit.hh"
#include "properties/NumericalFlat.hh"

namespace cadabra {

/**
 \ingroup properties

 Turns a symbol into an accent. Accented objects inherit all properties
 and indices from the objects which they wrap.
*/

class Accent : public PropertyInherit, public IndexInherit, public NumericalFlat, virtual public property {
	public:
		virtual std::string name() const;
};


}
