
#pragma once

#include "Algorithm.hh"

namespace cadabra {

	class expand_diracbar : public Algorithm {
		public:
			expand_diracbar(const Kernel&, Ex&);
			
			virtual bool     can_apply(iterator) override;
			virtual result_t apply(iterator&) override;		
	};

}
