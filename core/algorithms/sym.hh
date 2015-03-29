
#include "Algorithm.hh"
#include <vector>
#include "Combinatorics.hh"

class sym : virtual public Algorithm {
	public:
		sym(Kernel&, exptree&, exptree&, bool sign);

		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator& it) override;

	protected:
		exptree objects;
		bool    sign;

		std::vector<unsigned int>  argloc_2_treeloc;
		combin::combinations<unsigned int> raw_ints;

		result_t doit(sibling_iterator, sibling_iterator, bool);
};
