
#include "Algorithm.hh"

class product_rule : public Algorithm {
	public:
		product_rule(Kernel&, Ex&);

		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;

		sibling_iterator prodnode;
		unsigned int     number_of_indices;
};
