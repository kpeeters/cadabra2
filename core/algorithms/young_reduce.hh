#include <memory>
#include <gmpxx.h>

#include "Algorithm.hh"

namespace cadabra {

	namespace yr {

		using index_t = short;
		using adjform_t = std::vector<index_t>;

		struct ProjectedForm
		{
		public:
			using map_t = std::map<adjform_t, mpq_class>;
			using iterator = map_t::iterator;
			using const_iterator = map_t::const_iterator;

			// Check if 'other' is a linear multiple of 'this' and return
			// the numeric factor if so, otherwise returns 0
			mpq_class compare(const ProjectedForm& other) const;

			// Add all contributions from 'other' into 'this'
			void combine(const ProjectedForm& other, mpq_class factor = 1);

			// Multiply all terms by a constant factor
			void multiply(mpq_class k);

			// Remove all entries
			void clear();

			// Insert an adjform with given value, overwriting current value
			// if present
			void insert(adjform_t adjform, mpq_class value = 1);

			// Apply a Tableau row/column symmetry
			void apply_young_symmetry(const std::vector<index_t>& indices, bool antisymmetric);

			// Symmetrize in indices starting at the indice in 'positions' with each group
			// 'n_indices' long
			void apply_ident_symmetry(std::vector<index_t> positions, index_t n_indices);

			map_t data;
		};

		// Checks if 'lhs' and 'rhs' are identical up to index names
		bool check_structure(Ex::iterator lhs, Ex::iterator rhs);

		// Returns a list of iterators of the children of 'it' if it->name
		// is delim, otherwise returns a list containing 'it' as its only
		// entry. If 'pat' is specified, any terms not matching 'pat' are
		// ignored
		std::vector<Ex::iterator> split_ex(Ex::iterator it, const std::string& delim);
		std::vector<Ex::iterator> split_ex(Ex::iterator it, const std::string& delim, Ex::iterator pat);

		// Rewrite adjform type indices as dummy indices
		adjform_t collapse_dummy_indices(adjform_t adjform);

		// Rewrite dummy indices as adjform type indices
		adjform_t expand_dummy_indices(adjform_t adjform);
	}

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
		yr::ProjectedForm symmetrize(Ex::iterator it);

		Ex::iterator pat;
		yr::ProjectedForm pat_sym;

		// Rewrite 'it' as an adjacency form
		yr::adjform_t to_adjform(Ex::iterator it);

		// Return a unique negative index for each name
		yr::index_t get_free_index(nset_t::iterator name);
		std::vector<nset_t::iterator> index_map;
	};

}
