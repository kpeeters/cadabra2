#include "Algorithm.hh"

namespace cadabra
{
	void remove_common_terms(const Kernel& kernel, Ex& lhs, Ex& rhs);

	class young_reduce : public Algorithm
	{
	public:
		young_reduce(const Kernel& kernel, Ex& ex);

		virtual bool can_apply(iterator it);
		virtual result_t apply(iterator& it);

	private:
	};

}