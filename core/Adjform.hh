#pragma once

#include <vector>
#include <map>
#include <gmpxx.h>
#include <iosfwd>

#include "Storage.hh"

namespace cadabra {

	using AdjformIdx = short;
	using Adjform = std::vector<AdjformIdx>;

	// To ensure consistency when creating adjforms out of two
	// different Ex objects an IndexMap object is required which
	// keeps track of which numeric index represents which index
	// name 
	class IndexMap
	{
	public:
		// Create an Adjform object out of an expression
		Adjform to_adjform(Ex::iterator it);

	private:
		AdjformIdx get_free_index(nset_t::iterator name);
		std::vector<nset_t::iterator> data;
	};
	
	class AdjformEx
	{
	public:
		using index_t = short;
		using term_t = std::vector<index_t>;
		using map_t = std::map<term_t, mpq_class>;
		using iterator = map_t::iterator;
		using const_iterator = map_t::const_iterator;

		// Check if 'other' is a linear multiple of 'this' and return
		// the numeric factor if so, otherwise returns 0
		mpq_class compare(const AdjformEx& other) const;

		// Add all contributions from 'other' into 'this'
		void combine(const AdjformEx& other);
		void combine(const AdjformEx& other, mpq_class factor);

		// Multiply all terms by a constant factor
		void multiply(mpq_class k);

		// Return iterator to first term as a std::pair<const Adjform, mpq_class>
		iterator begin();
		const_iterator begin() const;

		// Return past the end iterator
		iterator end();
		const_iterator end() const;

		// Remove all entries
		void clear();

		// Number of entries
		size_t size() const;

		// True if there are no enries
		bool empty() const;

		// Sets the given term to value, creating/removing the term if required
		void set(term_t adjform, mpq_class value = 1);

		// Adds value to the given term, creating/removing the term if required
		void add(term_t adjform, mpq_class value = 1);

		// Symmetrize in the given indices
		// e.g. if the only term is abcd then
		//        apply_young_symmetry({0, 1, 2}, false) -> abcd + acbd + bacd + bcad + cabd + cbad
		//        apply_young_symmetry({2, 3, 4}, true) -> abcd - abdc - acbd + acdb - adcb + adbc
		void apply_young_symmetry(const std::vector<size_t>& indices, bool antisymmetric);

		// Symmetrize in indices starting at the indices in 'positions' with each group
		// 'n_indices' long
		// e.g. if the only term is abcdefg then apply_ident_symmetry({0, 2, 4}, 2) ->
		//      abcdefg + abefcdg + cdabefg + cdefabg + efabcdg + efcdabg
		void apply_ident_symmetry(std::vector<size_t> positions, size_t n_indices);

		// Symmetrize cyclically so abc -> abc + bca + cab
		void apply_cyclic_symmetry();

	private:
		map_t data;
	};
}

std::ostream& operator << (std::ostream& os, const cadabra::Adjform& adjform);
std::ostream& operator << (std::ostream& os, const cadabra::AdjformEx& adjex);
