#include "Algorithm.hh"

namespace cadabra
{
	class young_reduce : public Algorithm
	{
	public:
		young_reduce(const Kernel& kernel, Ex& ex, const Ex& pattern, bool search_permutations = false);

		virtual bool can_apply(iterator it);
		virtual result_t apply(iterator& it);

	private:
		void cleanup(iterator& it);

		result_t reduce(iterator& it, const std::vector<Ex::iterator>& its);
		result_t permute(iterator& it, const std::vector<Ex::iterator>& its);

		const Ex& pattern;
		bool search_permutations;
	};

}