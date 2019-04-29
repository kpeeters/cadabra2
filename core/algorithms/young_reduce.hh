#include <memory>
#include <gmpxx.h>

#include "Algorithm.hh"

namespace cadabra {

	class young_reduce : public Algorithm {
	public:
		using index_t = short;
		using indices_t = std::vector<index_t>;
		using tableau_t = std::vector<indices_t>;
		using terms_t = std::map<indices_t, mpq_class>;

		young_reduce(const Kernel& kernel, Ex& ex, const Ex& pattern);

		virtual bool can_apply(iterator it) override;
		virtual result_t apply(iterator& it) override;

	private:
		terms_t symmetrize(Ex::iterator);

		Ex pat;
		terms_t pat_decomp;
		std::vector<nset_t::iterator> index_map;
	};

}
