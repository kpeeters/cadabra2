#pragma once

#include <vector>
#include <map>
#include <iosfwd>
#include <cstdint>
#include <string>
#include "Compare.hh"
#include "Kernel.hh"
#include "Storage.hh"

namespace cadabra {

	class IndexMap;

	/// Representation of the index structure of a tensor monomial,
	/// using a storage format which resembles an adjacency matrix. The
	/// structure is stored as a vector of integers.  Negative integers
	/// denote free indices. Positive indices denote a contraction with
	/// an index at the indicated position.
	///
	/// Example:
	///
	///    A_{m n} B_{p n}   ->   -1 3 -2 1
	///       0 1     2 3
	///
	/// The tensor names themselves ('A' and 'B' above) are not stored.
	
	class Adjform
		{
		public:
			/// The maximal number of index slots is set by `value_type`: for a
			/// short, the maximal number of slots is 127.
			using value_type = short;
			using size_type = value_type;
			using difference_type = value_type;
			using array_type = std::vector<value_type>;
			using const_reference = array_type::const_reference;
			using const_iterator = array_type::const_iterator;

			Adjform();
			template <typename IndexIterator>
			Adjform(IndexIterator beg, IndexIterator end, IndexMap& index_map, const Kernel& kernel);
			template <typename ValueIterator>
			Adjform(ValueIterator beg, ValueIterator end, bool push_as_coordinates);

			const_iterator begin() const;
			const_iterator end() const;

			// Return the position of the given index starting from the given
			// offset (but returning the position from the actual start). Returns
			// this->size() if not found.
			size_type index_of(value_type index, size_type offset = 0) const;

			bool operator < (const Adjform& other) const;
			bool operator == (const Adjform& other) const;
			bool operator != (const Adjform& other) const;

			const_reference operator [] (size_type idx) const;

			size_type size() const;
			size_type max_size() const;
			bool empty() const;

			// Return true if the value at 'pos' is < 0
			bool is_free_index(size_type pos) const;
			// Return true if the value at pos is >= 0
			bool is_dummy_index(size_type pos) const;
	
			size_type n_free_indices() const;
			size_type n_dummy_indices() const;

			// Contract pairs of this type of index, e.g. with value=-1 '-1 -2 -3 -1' -> '3 -2 -3 0'.
			// Returns true if the index was found and replaced.
			bool resolve_dummy(value_type value);

			// Push value(s) to the end and contract it if a eqivalent is found, e.g. with
			// value=-1 '-1 -2 -3' -> '3 -2 -3 0'
			void push_index(value_type value);
			void push_indices(const Adjform& other);

			// PUsh value to the end, but do not attempt to contract it, e.g. with
			// value=-1 '-1 -2 -3' -> '-1 -2 -3 -1'
			void push_coordinate(value_type value);
			void push_coordinates(const Adjform& other);

			// Push an iterator by calling IndexMap::is_coordinate to see if push_index or 
			// push_coordinate should be used
			void push(Ex::iterator iterator, IndexMap& index_map, const Kernel& kernel);

			// Swap the values at positions a and b
			void swap(size_type a, size_type b);

			// Cycle the values by moving the last element to the front n times,
			// e.g. with n=1 '3 -2 -3 0' -> '1 0 -2 -3'
			void rotate(size_type n);

			// Sort indices so that all free indices are at the beginning and all contracted
			// pairs at the end and next to each other, e.g. '3 -2 -3 0' -> '-3 -2 3 2'
			void sort();

			// Produce a unique integer to represent the current permutation
			uint64_t to_lehmer_code() const;
			// The total number of possible permutations of values
			uint64_t max_lehmer_code() const;
			// String representation where indices are named 'a-z' in the order they appear
			std::string to_string() const;

		private:
			// Storage of an index configuration; has a maximal size
			// equal to size_type (not size_t).
			array_type data;
		};

	/// To ensure consistency when creating adjforms out of two
	/// different Ex objects an IndexMap object is required which
	/// keeps track of which numeric index represents which index
	/// name.
	
	class IndexMap
		{
		public:
			IndexMap(const Kernel& kernel);
			~IndexMap();
			// Return a negative integer for each unique index
			Adjform::value_type get_free_index(Ex::iterator index);
			// Return true if index has the Coordinate or Symbol property, or is an integer
			static bool is_coordinate(const Kernel& kernel, Ex::iterator index);
		private:
			std::unique_ptr<Ex_comparator> comp;
			std::unique_ptr<Ex> data;
		};

	/// Representation of a sum of tensor monomials, each having the
	/// same tensor names, but with different index positions. As with
	/// AdjForm, the names of the tensors are not stored, only the
	/// index structure and the coefficient of each term.
	
	class ProjectedAdjform
		{
		public:
			using integer_type = int32_t;
			using map_t = std::map<Adjform, integer_type>;
			using iterator = map_t::iterator;
			using const_iterator = map_t::const_iterator;

			ProjectedAdjform();
			ProjectedAdjform(const Adjform& adjform, const integer_type& value = 1);

			// Add all contributions from 'other' into 'this'
			void combine(const ProjectedAdjform& other); 
			void combine(const ProjectedAdjform& other, integer_type factor);
			ProjectedAdjform& operator += (const ProjectedAdjform& other);
			friend ProjectedAdjform operator + (ProjectedAdjform lhs, const ProjectedAdjform& rhs);

			// Multiply all terms by a scalar factor
			void multiply(const integer_type& k);
			ProjectedAdjform& operator *= (const integer_type& k);
			friend ProjectedAdjform operator * (ProjectedAdjform lhs, const integer_type& rhs);

			iterator begin();
			const_iterator begin() const;
			iterator end();
			const_iterator end() const;

			void clear(); // Remove all entries
			size_t size() const; // Number of entries
			size_t max_size() const; // Returns the number of terms there would be if fully symmetrized
			size_t n_indices() const; // Returns the number of indices each adjform has
			bool empty() const; // True if there are no entries

			// Get the value of the term, or zero if it doesn't exist
			const integer_type& get(const Adjform& adjform) const;
			// Sets the given term to value, creating/removing the term if required
			void set(const Adjform& adjform, const integer_type& value = 1);
			// Adds value to the given term, creating/removing the term if required
			void add(const Adjform& adjform, const integer_type& value = 1);

			// Symmetrise or anti-symmetrise in the given indices
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
			static integer_type zero;
		};

	template <typename IndexIterator>
	Adjform::Adjform(IndexIterator beg, IndexIterator end, IndexMap& index_map, const Kernel& kernel)
		{
		while (beg != end) {
			push(beg, index_map, kernel);
			++beg;
			}
		}

	template <typename ValueIterator>
	Adjform::Adjform(ValueIterator beg, ValueIterator end, bool push_as_coordinates)
		{
		while (beg != end) {
			auto val = *beg;
			if (push_as_coordinates)
				push_coordinate(val);
			else
				push_index(val);
			++beg;
			}
		}
	}


std::ostream& operator << (std::ostream& os, const cadabra::Adjform& adjform);
std::ostream& operator << (std::ostream& os, const cadabra::ProjectedAdjform& adjex);
