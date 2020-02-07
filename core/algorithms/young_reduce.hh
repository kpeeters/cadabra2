#include "Algorithm.hh"
#include "Adjform.hh"

namespace cadabra {

	class young_reduce : public Algorithm
	{
	public:
		young_reduce(const Kernel& kernel, Ex& ex, const Ex* pattern = nullptr);
		~young_reduce();

		// Attempt to set pattern to new_pat; return true if successful else false
		bool set_pattern(Ex::iterator new_pat);

		virtual bool can_apply(iterator it) override;
		virtual result_t apply(iterator& it) override;

	private:
		result_t apply_known(iterator& it);
		result_t apply_unknown(iterator& it);

		// Young project 'it' up to a numeric constant
		AdjformEx symmetrize(Ex::iterator it);

		Ex::iterator pat;
		AdjformEx pat_sym;

		IndexMap index_map;
	};

	class young_reduce_trace : public Algorithm
	{
	public:
		young_reduce_trace(const Kernel& kernel, Ex& ex);
		~young_reduce_trace();

		virtual bool can_apply(iterator it) override;
		virtual result_t apply(iterator& it) override;

	private:
		result_t apply_sum(iterator& it);
		result_t apply_trace(iterator& it);

		void cleanup_empty_traces(iterator it);

		struct CollectedTerm 
		{ 
			Ex::iterator it;
			Adjform names, indices; 
			mpq_class parent_multiplier;
			std::vector<size_t> pushes;
		};

		CollectedTerm collect_term(iterator it, mpq_class parent_multiplier);
		std::vector<CollectedTerm> collect_terms(iterator it);

		IndexMap index_map;
	};
}
