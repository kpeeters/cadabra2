#include "Algorithm.hh"

namespace cadabra
{
	class young_reduce : public Algorithm
	{
	public:
		enum class mode_t
		{
			// Only attempt to find combinations of terms which
			// equal zero
			eliminate,
			// Only attempt to find combinations of terms which
			// are a multiple of a term which exists in the expression
			collapse,
			// Attempt to find combinations of terms which are a multiple
			// of some index permutation
			permute,
			// Attempt all forms of reduction
			any
		};
		young_reduce(const Kernel& kernel, Ex& ex, const Ex& pattern, mode_t mode = mode_t::any);
		young_reduce(const Kernel& kernel, Ex& ex, const Ex& pattern, const std::string& mode);

		virtual bool can_apply(iterator it);
		virtual result_t apply(iterator& it);

	private:
		void cleanup(iterator& it);
		result_t delegate(iterator& it);

		result_t eliminate(iterator& it, const std::vector<Ex::iterator>& its);
		result_t collapse(iterator& it, const std::vector<Ex::iterator>& its);
		result_t permute(iterator& it, const std::vector<Ex::iterator>& its);
		result_t any(iterator& it, const std::vector<Ex::iterator>& its);

		mode_t mode;
		const Ex& pattern;

	};

}