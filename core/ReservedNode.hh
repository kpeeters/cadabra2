
#pragma once

#include "ExManip.hh"

namespace cadabra {

	namespace visit {
	
		class ReservedNode : public ExManip {
			public:
				ReservedNode(const Kernel&, Ex&, Ex::iterator);

				Ex::iterator node() const;
				
			protected:
				Ex::iterator   top;
		};

	}

}
	
