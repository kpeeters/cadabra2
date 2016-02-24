
#include "Algorithm.hh"

class unwrap : public Algorithm {
	public:
		unwrap(const Kernel&, Ex&);
		
		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;
};

