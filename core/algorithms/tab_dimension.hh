
#include "Algorithm.hh"
#include "YoungTab.hh"
#include "properties/Tableau.hh"
#include "properties/FilledTableau.hh"

namespace cadabra {

class tabdimension : public Algorithm {
	public:
      tabdimension(const Kernel&, Ex&);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);

		const Tableau *tab;
		const FilledTableau *ftab;
		int dimension;
};



}
