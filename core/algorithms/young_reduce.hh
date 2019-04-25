#include <memory>

#include "Algorithm.hh"

namespace cadabra {

	class young_reduce : public Algorithm {
		public:
			using index_t = short;
			using indices_t = std::vector<index_t>;
			using tableau_t = std::vector<indices_t>;
			using decomposition_t = std::map<Indices, mpq_class>;

			young_reduce(const Kernel& kernel, Ex& ex, const Ex& pattern);

			virtual bool can_apply(iterator it) override;
			virtual result_t apply(iterator& it) override;

		private:
			class Flyweight;
			std::unique_ptr<Flyweight> name_map, index_map;

			class Tensor;
			std::unique_ptr<Tensor> pat;
		};

	}
