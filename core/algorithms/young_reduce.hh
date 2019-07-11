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
		using names_t = std::vector<std::pair<nset_t::iterator, size_t>>;

		young_reduce(const Kernel& kernel, Ex& ex, const Ex& pattern);
		~young_reduce();

		virtual bool can_apply(iterator it) override;
		virtual result_t apply(iterator& it) override;

	private:
		result_t apply_sum(iterator& it);
		result_t apply_monoterm(iterator& it);

		terms_t symmetrize(Ex::iterator);
		
		Ex pat;
		terms_t pat_decomp;

		struct IndexMap;
		std::unique_ptr<IndexMap> index_map;
	};

}
