
#pragma once

#include "Algorithm.hh"

namespace cadabra {

class order : virtual public Algorithm {
	public:
		order(const Kernel&, Ex&, Ex& objs, bool ac);

		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;

	protected:
		Ex objects;
		bool    anticomm;
};

}
