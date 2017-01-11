
#include "Algorithm.hh"

namespace cadabra {

	class flatten_sum : public Algorithm {
		public:
			flatten_sum(const Kernel&, Ex&);
			
			virtual bool     can_apply(iterator);
			virtual result_t apply(iterator&);
			
			bool make_consistent_only;
	};

}
