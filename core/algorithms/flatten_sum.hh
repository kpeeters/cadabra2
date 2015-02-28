
#include "Algorithm.hh"

class flatten_sum : public Algorithm {
	public:
		flatten_sum(Kernel&, exptree&);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);

		bool make_consistent_only;
};

