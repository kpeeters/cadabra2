#pragma once

#include <vector>
#include <map>
#include <gmpxx.h>
#include <iosfwd>
#include <cstdint>
#include <string>
#include "Compare.hh"
#include "Hash.hh"
#include "Kernel.hh"
#include "Storage.hh"

namespace cadabra {

	// Return true if 'it' has children which are indices and not
	// registered as Symbol or Coordinate.
	bool has_indices(const Kernel& kernel, Ex::iterator it);
	bool is_index(const Kernel& kernel, Ex::iterator it);

	class IndexMap;

	class Adjform
	{
	public:
		using value_type = short;
		using size_type = value_type;
		using difference_type = value_type;
		using array_type = std::vector<value_type>;
		using const_reference = array_type::const_reference;
		using const_iterator = array_type::const_iterator;

		Adjform();
		Adjform(Ex::iterator it, IndexMap& index_map, const Kernel& kernel);

		const_iterator begin() const;
		const_iterator end() const;

		bool operator < (const Adjform& other) const;
		bool operator == (const Adjform& other) const;
		bool operator != (const Adjform& other) const;

		Adjform& operator *= (const Adjform& other);
		friend Adjform operator * (Adjform lhs, const Adjform& rhs);

		const_reference operator [] (size_type idx) const;

		size_type size() const;
		size_type max_size() const;
		bool empty() const;

		size_type n_free_indices() const;
		size_type n_dummy_indices() const;

		void push_back(value_type value);
		void swap(size_type a, size_type b);
		void rotate(size_type n);

		uint64_t to_lehmer_code() const;
		uint64_t max_lehmer_code() const;
		std::string to_string() const;

		static bool compare(Ex::iterator a, Ex::iterator b, const Kernel& kernel);

	private:
		array_type data;
	};

	bool is_free_index(Adjform::value_type idx);
	bool is_dummy_index(Adjform::value_type idx);

	// To ensure consistency when creating adjforms out of two
	// different Ex objects an IndexMap object is required which
	// keeps track of which numeric index represents which index
	// name 
	class IndexMap
	{
	public:
		IndexMap(const Kernel& kernel);
		~IndexMap();
		Adjform::value_type get_free_index(Ex::iterator index);
	private:
		std::unique_ptr<Ex_comparator> comp;
		std::unique_ptr<Ex> data;
	};

	class AdjformEx
	{
	public:
		using integer_type = int32_t;
		using map_t = std::map<Adjform, integer_type>;
		using iterator = map_t::iterator;
		using const_iterator = map_t::const_iterator;

		AdjformEx();
		AdjformEx(const Adjform& adjform, const integer_type& value = 1, const Ex& prefactor = Ex());
		AdjformEx(const Adjform& adjform, const integer_type& value, Ex::iterator prefactor);
		AdjformEx(Ex& tr, Ex::iterator it, IndexMap& index_map, const Kernel& kernel);

		AdjformEx& operator += (const AdjformEx& other);
		friend AdjformEx operator + (AdjformEx lhs, const AdjformEx& rhs);
		AdjformEx& operator *= (const AdjformEx& other);
		friend AdjformEx operator * (AdjformEx lhs, const AdjformEx& rhs);
		AdjformEx& operator *= (const Adjform& other);
		friend AdjformEx operator * (AdjformEx lhs, const Adjform& rhs);

		// Check if 'other' is a linear multiple of 'this' and return
		// the numeric factor if so, otherwise returns 0
		integer_type compare(const AdjformEx& other) const;

		void combine(const AdjformEx& other); // Add all contributions from 'other' into 'this'
		void combine(const AdjformEx& other, integer_type factor);
		void multiply(const integer_type& k); // Multiply all terms by a constant factor

		iterator begin();
		const_iterator begin() const;
		iterator end();
		const_iterator end() const;

		void clear(); // Remove all entries
		size_t size() const; // Number of entries
		size_t max_size() const; // Returns the number of terms there would be if fully antisymmetrized
		size_t n_indices() const; // Returns the number of indices each adform has
		bool empty() const; // True if there are no entries

		const Ex& get_prefactor_ex() const;
		Ex& get_prefactor_ex();
		const Ex& get_tensor_ex() const;
		Ex& get_tensor_ex();

		// Get the value of the term, or zero if it doesn't exist
		const integer_type& get(const Adjform& adjform) const;
		// Sets the given term to value, creating/removing the term if required
		void set(const Adjform& adjform, const integer_type& value = 1);
		// Adds value to the given term, creating/removing the term if required
		void add(const Adjform& adjform, const integer_type& value = 1);

		// Symmetrize in the given indices
		// e.g. if the only term is abcd then
		//        apply_young_symmetry({0, 1, 2}, false) -> abcd + acbd + bacd + bcad + cabd + cbad
		//        apply_young_symmetry({2, 3, 4}, true) -> abcd - abdc - acbd + acdb - adcb + adbc
		void apply_young_symmetry(const std::vector<size_t>& indices, bool antisymmetric);

		// Symmetrize in indices starting at the indices in 'positions' with each group
		// 'n_indices' long.
		// e.g. if the only term is abcdefg then apply_ident_symmetry({0, 2, 4}, 2) ->
		//      abcdefg + abefcdg + cdabefg + cdefabg + efabcdg + efcdabg
		void apply_ident_symmetry(const std::vector<size_t>& positions, size_t n_indices);
		void apply_ident_symmetry(const std::vector<size_t>& positions, size_t n_indices, const std::vector<std::vector<int>>& commutation_matrix);

		// Symmetrize cyclically so abc -> abc + bca + cab
		void apply_cyclic_symmetry();

	private:
		// Unsafe (but faster) versions of the public functions
		void set_(const Adjform& adjform, const integer_type& value = 1);
		void add_(const Adjform& adjform, const integer_type& value = 1);
		map_t data;
		Ex prefactor;
		Ex tensor;
		static integer_type zero;
	};
}

std::ostream& operator << (std::ostream& os, const cadabra::Adjform& adjform);
std::ostream& operator << (std::ostream& os, const cadabra::AdjformEx& adjex);
