
#include "Algorithm.hh"

namespace cadabra {

class product_rule : public Algorithm {
	public:
		product_rule(const Kernel&, Ex&);

		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;

		sibling_iterator prodnode;
		unsigned int     number_of_indices;
};

}
