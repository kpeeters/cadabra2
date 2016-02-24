
#pragma once

#include "Algorithm.hh"
#include "YoungTab.hh"

class young_project : public Algorithm {
	public:
		young_project(const Kernel&, Ex&);
		young_project(const Kernel&, Ex&, const std::vector<int>& shape, const std::vector<int>& indices);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);		

		// Boxes represent tensor slots, the values in them
		// give the location in the tensor.
		typedef yngtab::filled_tableau<unsigned int> pos_tab_t;
		typedef yngtab::filled_tableau<iterator>     name_tab_t;
		pos_tab_t  tab;
		name_tab_t nametab;
		combin::symmetriser<unsigned int> sym;

		// Implicit antisymmetry; used by tableaux.cc only so far. These
		// store index locations, so these are "values" from the 
		// combinatorics.hh point of view.
		combin::combinations<unsigned int>::permuted_sets_t asym_ranges;
		bool remove_traces;

	private:
		iterator nth_index_node(iterator, unsigned int);
};

