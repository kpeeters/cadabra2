
#pragma once

#include "Algorithm.hh"

namespace cadabra {

class untrace : public Algorithm {
	public:
		untrace(const Kernel&, Ex&);

		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;
};

}
