
#include "Algorithm.hh"
#include "YoungTab.hh"

class tab_basics : public Algorithm {
	public:
		tab_basics(const Kernel&, Ex&);

		typedef yngtab::filled_tableau<unsigned int> uinttab_t;
		typedef yngtab::tableaux<uinttab_t>          uinttabs_t;

		/// Convert an Ex to a numerical Young tableau, using num_to_it below.
		void tree_to_numerical_tab(iterator, uinttab_t& );
		unsigned int find_obj(const Ex& other);

		/// The inverse, converting tableaux to Ex objects attached as children of the iterator.
		void tabs_to_tree(uinttabs_t&, iterator, iterator, bool even_only);
		void tabs_to_singlet_rules(uinttabs_t&, iterator);
		
		std::vector<Ex::iterator> num_to_it;
};
