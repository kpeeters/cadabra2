
#pragma once

#include "Storage.hh"
#include "Props.hh"

namespace cadabra {

	/// \ingroup core
	///
	/// An iterator which iterates over indices even if they are at lower
	/// levels, i.e. taking into account the "Inherit" property of
	/// nodes. Needs access to Properties in the current scope in order to
	/// determine which objects are indices.

	class index_iterator : public Ex::iterator_base {
		public:
			index_iterator(const Properties&);
			index_iterator(const index_iterator&);

			static index_iterator create(const Properties&, const iterator_base&);

			static index_iterator begin(const Properties&, const iterator_base&, int offset=0);
			static index_iterator end(const Properties&, const iterator_base&);

			index_iterator& operator=(const index_iterator&);
			bool    operator==(const index_iterator&) const;
			bool    operator!=(const index_iterator&) const;
			index_iterator&  operator++();
			index_iterator   operator++(int);
			index_iterator&  operator+=(unsigned int);

			Ex::iterator halt, walk, roof;
		private:
			const Properties *properties;

			bool is_index(Ex::iterator) const;
		};

	struct iter_indices
	{
		struct iterator {
			using value_type = index_iterator;
			using difference_type = ptrdiff_t;
			using reference = value_type&;
			using const_reference = const value_type&;
			using pointer = value_type*;
			using const_pointer = const value_type*;
			using iterator_category = std::input_iterator_tag;

			iterator(index_iterator it) : it(it) {}

			bool operator == (const iterator& other) { return it == other.it; }
			bool operator != (const iterator& other) { return !(*this == other); }
			reference operator* () { return it; }
			pointer operator-> () { return &it; }
			reference operator ++ () { return ++it; }
			value_type operator ++ (int) { return it++; }


			index_iterator it;
		};

		iter_indices(const Properties& properties, Ex::iterator it)
			: properties(properties)
			, it(it) { }

		iterator begin() { return index_iterator::begin(properties, it); }
		iterator begin(int offset) { return index_iterator::begin(properties, it, offset); }
		iterator end() { return index_iterator::end(properties, it); }
		size_t size() { return std::distance(begin(), end()); }

	private:
		Ex::iterator it;
		const Properties& properties;
	};
	//size_t number_of_indices(const Properties&, Ex::iterator);
	}
