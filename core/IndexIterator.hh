
#pragma once

#include "Storage.hh"
#include "Props.hh"

/// \ingroup core
///
/// An iterator which iterates over indices even if they are at lower
/// levels, i.e. taking into account the "Inherit" property of
/// nodes. Needs access to Properties in the current scole in order to
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

size_t number_of_indices(const Properties&, Ex::iterator);
