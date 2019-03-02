
#include "Algorithm.hh"
#include <vector>
#include "Combinatorics.hh"

namespace cadabra {

	class sym : virtual public Algorithm {
		public:
			sym(const Kernel&, Ex&, Ex&, bool sign);

			virtual bool     can_apply(iterator) override;
			virtual result_t apply(iterator& it) override;

		protected:
			Ex objects;
			bool    sign;

			std::vector<unsigned int>  argloc_2_treeloc;
			combin::combinations<unsigned int> raw_ints;

			result_t doit(iterator&, bool);
		};

	}
