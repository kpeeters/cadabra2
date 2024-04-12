/*

Cadabra: a field-theory motivated computer algebra system.
Copyright (C) 2001-2011  Kasper Peeters <kasper.peeters@aei.mpg.de>

	This program is free software: you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

/*
- TODO: has_nullifying trace is wrong, but needs to be merged with the
        input_asym code in order to be more useful.

*/

#pragma once

#include <cstddef>
#include <iostream>
#include <iterator>
#include <vector>
#include <list>
#include <gmpxx.h>
#include "Combinatorics.hh"
#include <cstddef>

typedef mpz_class yngint_t;
typedef mpq_class yngrat_t;

/// Generic Young tableaux routines
namespace yngtab {

	// The tableau_base is the abstract interface; does not depend on the
	// actual storage format.

	class tableau_base {
		public:
			tableau_base();
			virtual ~tableau_base();
			virtual unsigned int number_of_rows() const=0;
			virtual unsigned int row_size(unsigned int row) const=0;
			virtual unsigned int column_size(unsigned int col) const; // FIXME: maybe make pure virt too
			virtual void         add_box(unsigned int row)=0;
			virtual void         remove_box(unsigned int row)=0;
			virtual void         add_row(unsigned int row_size);
			virtual void         clear()=0;

			yngrat_t             multiplicity;    // also keeps track of signs
			int                  selfdual_column; // -n, 0, n  for antiselfdual, no, selfdual (count from 1)
			yngint_t             dimension(unsigned int) const;
			unsigned long        hook_length(unsigned int row, unsigned int col) const;
			yngint_t             hook_length_prod() const;
		};

	class tableau : public tableau_base {
		public:
			tableau();
			tableau(const tableau&);
			
			virtual ~tableau();
			virtual unsigned int number_of_rows() const;
			virtual unsigned int row_size(unsigned int row) const;
			virtual void         add_box(unsigned int row);
			virtual void         remove_box(unsigned int row);
			virtual void         clear();

			tableau& operator=(const tableau&);
		private:
			std::vector<int> rows;
		};

	template<class T>
	class tableaux;

	template<class T>
	class filled_tableau : public tableau {
		public:
			typedef T value_type;

			filled_tableau();
			filled_tableau(const filled_tableau<T>&);
			
			virtual ~filled_tableau();
			virtual unsigned int number_of_rows() const;
			virtual unsigned int row_size(unsigned int row) const;
			virtual void         add_box(unsigned int row);
			virtual void         remove_box(unsigned int row);
			std::pair<int, int>  find(const T&) const;
			virtual void         clear();

			void                 copy_shape(const tableau&);

			T&                   operator()(unsigned int row, unsigned int col);
			const T&             operator()(unsigned int row, unsigned int col) const;
			const T&             operator[](unsigned int boxnum) const;
			void                 add_box(unsigned int rownum, T val);
			void                 swap_columns(unsigned int c1, unsigned int c2);

			bool                 compare_without_multiplicity(const filled_tableau<T>& other) const;
			bool                 has_nullifying_trace() const;
			void                 sort_within_columns();
			void                 sort_columns();
			/// Sort equal-length columns and sort within columns.
			void                 canonicalise();
			std::pair<int, int>  nonstandard_loc() const;
			template<class StrictWeakOrdering> void sort_within_columns(StrictWeakOrdering comp);
			template<class StrictWeakOrdering> void sort_columns(StrictWeakOrdering comp);
			template<class StrictWeakOrdering> void canonicalise(StrictWeakOrdering comp, bool only_col_ex=false);
			void                 projector(combin::symmetriser<T>&, bool modulo_monoterm=false) const;
			void                 projector(combin::symmetriser<T>&, combin::range_vector_t&) const;
			yngrat_t             projector_normalisation() const;

			filled_tableau<T>& operator=(const filled_tableau<T>&);

			class iterator_base {
				public:
					typedef T                               value_type;
					typedef T*                              pointer;
					typedef T&                              reference;
					typedef size_t                          size_type;
					typedef ptrdiff_t                       difference_type;
					typedef std::random_access_iterator_tag iterator_category;
				};

			class const_iterator_base {
			public:
				typedef T                               value_type;
				typedef const T* pointer;
				typedef const T& reference;
				typedef size_t                          size_type;
				typedef ptrdiff_t                       difference_type;
				typedef std::random_access_iterator_tag iterator_category;
			};

			class const_iterator;
			class in_column_iterator;
			class in_column_const_iterator;
			class in_row_iterator;
			class in_row_const_iterator;						
			
			/// An iterator which stays inside a given column of a tableau.
			class in_column_iterator : public iterator_base {
				public:
					in_column_iterator(unsigned int r, unsigned int c, filled_tableau<T> *);
					T&                  operator*() const;
					T*                  operator->() const;
					in_column_iterator& operator++();
					in_column_iterator  operator++(int);
					in_column_iterator& operator--();
					in_column_iterator  operator--(int);
					in_column_iterator  operator+(unsigned int) const;
					in_column_iterator  operator-(unsigned int) const;
					in_column_iterator& operator+=(unsigned int);
					in_column_iterator& operator-=(unsigned int);
					T&                  operator[](int n) const;
					bool                operator<(const in_column_iterator& other) const;
					bool                operator>(const in_column_iterator& other) const;
					bool                operator<=(const in_column_iterator& other) const;
					bool                operator>=(const in_column_iterator& other) const;
					ptrdiff_t           operator-(const in_column_iterator&) const;
					bool                operator==(const in_column_iterator&) const;
					bool                operator!=(const in_column_iterator&) const;

					friend class filled_tableau<T>;
					friend class filled_tableau<T>::in_column_const_iterator;
				private:
					filled_tableau<T> *tab;
					unsigned int       column_number, row_number;
				};

			/// A const iterator which stays inside a given column of a tableau.
			class in_column_const_iterator : public const_iterator_base {
			public:
				in_column_const_iterator(unsigned int r, unsigned int c, const filled_tableau<T>*);
				in_column_const_iterator(const in_column_iterator& other);
				const T& operator*() const;
				const T* operator->() const;
				in_column_const_iterator& operator++();
				in_column_const_iterator  operator++(int);
				in_column_const_iterator& operator--();
				in_column_const_iterator  operator--(int);
				in_column_const_iterator  operator+(unsigned int) const;
				in_column_const_iterator  operator-(unsigned int) const;
				in_column_const_iterator& operator+=(unsigned int);
				in_column_const_iterator& operator-=(unsigned int);
				bool                operator<(const in_column_const_iterator& other) const;
				bool                operator>(const in_column_const_iterator& other) const;
				bool                operator<=(const in_column_const_iterator& other) const;
				bool                operator>=(const in_column_const_iterator& other) const;
				ptrdiff_t           operator-(const in_column_const_iterator&) const;
				bool                operator==(const in_column_const_iterator&) const;
				bool                operator!=(const in_column_const_iterator&) const;

				friend class filled_tableau<T>;
			private:
				const filled_tableau<T>* tab;
				unsigned int       column_number, row_number;
			};

			/// An iterator which stays inside a given row of a tableau.
			class in_row_iterator : public iterator_base {
			public:
				in_row_iterator(unsigned int r, unsigned int c, filled_tableau<T>*);
				T& operator*() const;
				T* operator->() const;
				in_row_iterator& operator++();
				in_row_iterator  operator++(int);
				in_row_iterator& operator--();
				in_row_iterator  operator--(int);
				in_row_iterator  operator+(unsigned int) const;
				in_row_iterator  operator-(unsigned int) const;
				in_row_iterator& operator+=(unsigned int);
				in_row_iterator& operator-=(unsigned int);
				bool                operator<(const in_row_iterator& other) const;
				bool                operator>(const in_row_iterator& other) const;
				bool                operator<=(const in_row_iterator& other) const;
				bool                operator>=(const in_row_iterator& other) const;
				ptrdiff_t           operator-(const in_row_iterator&) const;
				bool                operator==(const in_row_iterator&) const;
				bool                operator!=(const in_row_iterator&) const;

				friend class filled_tableau<T>;
				friend class filled_tableau<T>::in_row_const_iterator;
			private:
				filled_tableau<T>* tab;
				unsigned int       column_number, row_number;
			};

			class in_row_const_iterator : public const_iterator_base {
			public:
				in_row_const_iterator(unsigned int r, unsigned int c, const filled_tableau<T>*);
				in_row_const_iterator(const in_row_iterator& other);
				const T& operator*() const;
				const T* operator->() const;
				in_row_const_iterator& operator++();
				in_row_const_iterator  operator++(int);
				in_row_const_iterator& operator--();
				in_row_const_iterator  operator--(int);
				in_row_const_iterator  operator+(unsigned int) const;
				in_row_const_iterator  operator-(unsigned int) const;
				in_row_const_iterator& operator+=(unsigned int);
				in_row_const_iterator& operator-=(unsigned int);
				bool                operator<(const in_row_const_iterator& other) const;
				bool                operator>(const in_row_const_iterator& other) const;
				bool                operator<=(const in_row_const_iterator& other) const;
				bool                operator>=(const in_row_const_iterator& other) const;
				ptrdiff_t           operator-(const in_row_const_iterator&) const;
				bool                operator==(const in_row_const_iterator&) const;
				bool                operator!=(const in_row_const_iterator&) const;

				friend class filled_tableau<T>;
			private:
				const filled_tableau<T>* tab;
				unsigned int       column_number, row_number;
			};

			/// An iterator over all boxes of a tableau, left to right, top to bottom.
			class iterator : public iterator_base {
				public:
					iterator(unsigned int r, unsigned int c, filled_tableau<T> *);
					T&                  operator*() const;
					T*                  operator->() const;
					iterator&           operator++();
					iterator            operator++(int);
					iterator&           operator--();
					iterator            operator--(int);
					iterator            operator+(unsigned int) const;
					iterator            operator-(unsigned int) const;
					iterator&           operator+=(unsigned int);
					iterator&           operator-=(unsigned int);
					bool                operator<(const iterator& other) const;
					bool                operator>(const iterator& other) const;
					ptrdiff_t           operator-(const iterator&) const;
					bool                operator==(const iterator&) const;
					bool                operator!=(const iterator&) const;

					friend class filled_tableau<T>;
					friend class filled_tableau<T>::const_iterator;
				private:
					filled_tableau<T> *tab;
					unsigned int       column_number, row_number;
				};

			class const_iterator : public const_iterator_base {
			public:
				const_iterator(unsigned int r, unsigned int c, const filled_tableau<T>*);
				const_iterator(const iterator& other);
				const T& operator*() const;
				const T* operator->() const;
				const_iterator&				operator++();
				const_iterator            operator++(int);
				const_iterator&				operator--();
				const_iterator            operator--(int);
				const_iterator            operator+(unsigned int) const;
				const_iterator            operator-(unsigned int) const;
				const_iterator&				 operator+=(unsigned int);
				const_iterator&				operator-=(unsigned int);
				bool			               operator<(const const_iterator& other) const;
				bool					        operator>(const const_iterator& other) const;
				ptrdiff_t					  operator-(const const_iterator&) const;
				bool			             operator==(const const_iterator&) const;
				bool					        operator!=(const const_iterator&) const;

				friend class filled_tableau<T>;
			private:
				const filled_tableau<T>* tab;
				unsigned int       column_number, row_number;
			};


			in_column_iterator   begin_column(unsigned int column_number);
			in_column_iterator   end_column(unsigned int column_number);
			in_column_const_iterator   begin_column(unsigned int column_number) const;
			in_column_const_iterator   end_column(unsigned int column_number) const;
			in_column_const_iterator   cbegin_column(unsigned int column_number) const;
			in_column_const_iterator   cend_column(unsigned int column_number) const;
			in_row_iterator		begin_row(unsigned int row_number);
			in_row_iterator		end_row(unsigned int row_number);
			in_row_const_iterator		begin_row(unsigned int row_number) const;
			in_row_const_iterator		end_row(unsigned int row_number) const;
			in_row_const_iterator		cbegin_row(unsigned int row_number) const;
			in_row_const_iterator		cend_row(unsigned int row_number) const;
			iterator begin();
			iterator end();
			const_iterator             begin() const;
			const_iterator             end() const;
			const_iterator             cbegin() const;
			const_iterator             cend() const;

			template<class OutputIterator>
			OutputIterator       Garnir_set(OutputIterator, unsigned int, unsigned int) const;
		private:
			typedef std::vector<T>       box_row;
			typedef std::vector<box_row> row_stack;
			row_stack rows;
		};

	template<class T>
	class tableaux {
		public:
			yngint_t         total_dimension(unsigned int dim);
			void             remove_nullifying_traces();
			/// Put the set of tableaux into standard form by using Garnir symmetries.
			/// Return value indicates whether the tableaux were already all in standard form.
			bool             standard_form();
			void             add_tableau(const T&);
			void             symmetrise(const T& tabsym);

			typedef std::list<T> tableau_container_t;
			tableau_container_t  storage;

			typedef std::back_insert_iterator<tableau_container_t> back_insert_iterator;

			back_insert_iterator get_back_insert_iterator();
		};

	bool legal_box(const std::vector<std::pair<int,int> >& prev,
	               const std::vector<std::pair<int,int> >& ths,
	               int colpos, int trypos);

	// --------------------------------------


	template<class T>
	typename tableaux<T>::back_insert_iterator tableaux<T>::get_back_insert_iterator()
		{
		return back_insert_iterator(storage);
		}

	template<class T>
	void tableaux<T>::remove_nullifying_traces()
		{
		typename tableau_container_t::iterator it=storage.begin();
		while(it!=storage.end()) {
			if(it->has_nullifying_trace())
				it=storage.erase(it);
			else ++it;
			}
		}

	template<class T>
	void tableaux<T>::symmetrise(const T&)
		{
		//
		//	typename tableau_container_t::iterator thetab=storage.begin();
		//	while(thetab!=storage.end()) {
		//		(*thetab).sort_columns();
		//		std::pair<int,int> where=(*thetab).nonstandard_loc();
		//		if(where.first!=-1) {
		//			combinations<typename T::value_type> com;
		//

		/*
		FIXME: we should have two LR_tensor routines, because if you do 'alltabs', you should
		keep track of which boxes came from tableau 2. So do a LR_tensor with numbered boxes,
			and then after the LR_tensor apply the symmetries of the original tableaux, put back
		the original index names, sort columns and determine whether the tableau is identically
			non-zero. Then add to the product.

		Another issue: adding to tableaux should have an option to not insert doubles.

			There was something third, forgotten...
		*/
		}

	template<class T>
	void filled_tableau<T>::copy_shape(const tableau& other)
		{
		rows.clear();
		for(unsigned int r=0; r<other.number_of_rows(); ++r) {
			rows.push_back(box_row(other.row_size(r)));
			}
		tableau::operator=(other);
		}

	template<class T>
	bool filled_tableau<T>::compare_without_multiplicity(const filled_tableau<T>& other) const
		{
		return (rows==other.rows);
		}

	template<class T>
	bool filled_tableau<T>::has_nullifying_trace() const
		{
		return false;

		// Old, probably incorrect code:
		//
		//	for(unsigned int r1=0; r1<number_of_rows(); ++r1) {
		//		for(unsigned c1=0; c1<row_size(r1); ++c1) {
		//			for(unsigned int c2=c1+1; c2<row_size(r1); ++c2) {
		//				// (r1,c1) and (r1,c2)
		//				for(unsigned int c3=0; c3<row_size(0); ++c3) {
		//					unsigned int r3=0;
		//					while(r3<number_of_rows()-1 && c3<row_size(r3)) {
		//						unsigned int r4=r3+1;
		//						while(r4<number_of_rows() && c3<row_size(r4)) {
		//							if((rows[r1][c1]==rows[r3][c3] && rows[r1][c2]==rows[r4][c3]) ||
		//								(rows[r1][c1]==rows[r4][c3] && rows[r1][c2]==rows[r3][c3]) )
		//								return true;
		//							++r4;
		//							}
		//						++r3;
		//						}
		//					}
		//				}
		//			}
		//		}
		//	return false;
		}

	template<class T>
	std::pair<int, int> filled_tableau<T>::find(const T& obj) const
		{
		for(unsigned int ir=0; ir<rows.size(); ++ir) {
			for(unsigned int ic=0; ic<rows[ir].size(); ++ic) {
				if(rows[ir][ic]==obj)
					return std::pair<int,int>(ir, ic);
				}
			}
		return std::pair<int,int>(-1,-1);
		}

	template<class T>
	void filled_tableau<T>::sort_within_columns()
		{
		std::less<T> comp;
		sort_within_columns(comp);
		}

	template<class T>
	void filled_tableau<T>::sort_columns()
		{
		std::less<T> comp;
		sort_columns(comp);
		}

	template<class T>
	void filled_tableau<T>::canonicalise()
		{
		std::less<T> comp;
		canonicalise(comp);
		}

	template<class T>
	template<class StrictWeakOrdering>
	void filled_tableau<T>::sort_within_columns(StrictWeakOrdering comp)
		{
		filled_tableau<T> tmp(*this);
		if(number_of_rows()==0) return;
		for(unsigned int c=0; c<row_size(0); ++c) {
			std::sort(begin_column(c), end_column(c), comp);
			multiplicity*=combin::ordersign(begin_column(c), end_column(c), tmp.begin_column(c), tmp.end_column(c));
			}
		}

	template<class T>
	template<class StrictWeakOrdering>
	void filled_tableau<T>::sort_columns(StrictWeakOrdering comp)
		{
		for(unsigned int c1=0; c1<row_size(0); ++c1) {
			for(unsigned int c2=c1; c2<row_size(0); ++c2) {
				if(column_size(c1)==column_size(c2)) {
					if(comp((*this)(0,c2), (*this)(0,c1)))
						swap_columns(c1,c2);
					}
				}
			}
		}

	template<class T>
	template<class StrictWeakOrdering>
	void filled_tableau<T>::canonicalise(StrictWeakOrdering comp, bool only_col_ex)
		{
		if(!only_col_ex)
			sort_within_columns(comp);
		sort_columns(comp);
		}

	//---------------------------------------------------------------------------
	// in_column_iterator

	template<class T>
	filled_tableau<T>::in_column_iterator::in_column_iterator(unsigned int r, unsigned int c, filled_tableau<T> *t)
		: tab(t), column_number(c), row_number(r)
		{
		}

	template<class T>
	typename filled_tableau<T>::in_column_iterator filled_tableau<T>::in_column_iterator::operator+(unsigned int n) const
		{
		typename filled_tableau<T>::in_column_iterator it2(*this);
		it2+=n;
		return it2;
		}

	template<class T>
	typename filled_tableau<T>::in_column_iterator filled_tableau<T>::in_column_iterator::operator-(unsigned int n) const
		{
		typename filled_tableau<T>::in_column_iterator it2(*this);
		it2-=n;
		return it2;
		}

	template<class T>
	ptrdiff_t filled_tableau<T>::in_column_iterator::operator-(const in_column_iterator& other) const
		{
		return row_number-other.row_number;
		}

	template<class T>
	T& filled_tableau<T>::in_column_iterator::operator[](int n) const
		{
		return (*tab)(row_number + n, column_number);
		}

	template<class T>
	T& filled_tableau<T>::in_column_iterator::operator*() const
		{
		return (*tab)(row_number,column_number);
		}

	template<class T>
	T* filled_tableau<T>::in_column_iterator::operator->() const
		{
		return &((*tab)(row_number,column_number));
		}

	template<class T>
	typename filled_tableau<T>::in_column_iterator& filled_tableau<T>::in_column_iterator::operator++()
		{
		++row_number;
		return (*this);
		}

	template<class T>
	typename filled_tableau<T>::in_column_iterator& filled_tableau<T>::in_column_iterator::operator+=(unsigned int n)
		{
		row_number+=n;
		return (*this);
		}

	template<class T>
	typename filled_tableau<T>::in_column_iterator& filled_tableau<T>::in_column_iterator::operator--()
		{
		--row_number;
		return (*this);
		}

	template<class T>
	typename filled_tableau<T>::in_column_iterator filled_tableau<T>::in_column_iterator::operator--(int)
		{
		in_column_iterator tmp(*this);
		--row_number;
		return tmp;
		}

	template<class T>
	typename filled_tableau<T>::in_column_iterator filled_tableau<T>::in_column_iterator::operator++(int)
		{
		in_column_iterator tmp(*this);
		++row_number;
		return tmp;
		}

	template<class T>
	typename filled_tableau<T>::in_column_iterator& filled_tableau<T>::in_column_iterator::operator-=(unsigned int n)
		{
		row_number-=n;
		return (*this);
		}

	template<class T>
	bool filled_tableau<T>::in_column_iterator::operator==(const in_column_iterator& other) const
		{
		if(tab==other.tab && row_number==other.row_number && column_number==other.column_number)
			return true;
		return false;
		}

	template<class T>
	bool filled_tableau<T>::in_column_iterator::operator<=(const in_column_iterator& other) const
		{
		if(row_number<=other.row_number) return true;
		return false;
		}

	template<class T>
	bool filled_tableau<T>::in_column_iterator::operator>=(const in_column_iterator& other) const
		{
		if(row_number>=other.row_number) return true;
		return false;
		}

	template<class T>
	bool filled_tableau<T>::in_column_iterator::operator<(const in_column_iterator& other) const
		{
		if(row_number<other.row_number) return true;
		return false;
		}

	template<class T>
	bool filled_tableau<T>::in_column_iterator::operator>(const in_column_iterator& other) const
		{
		if(row_number>other.row_number) return true;
		return false;
		}

	template<class T>
	bool filled_tableau<T>::in_column_iterator::operator!=(const in_column_iterator& other) const
		{
		return !((*this)==other);
		}

	//---------------------------------------------------------------------------
// in_column_const_iterator

	template<class T>
	filled_tableau<T>::in_column_const_iterator::in_column_const_iterator(unsigned int r, unsigned int c, const filled_tableau<T>* t)
		: tab(t), column_number(c), row_number(r)
	{
	}

	template<class T>
	filled_tableau<T>::in_column_const_iterator::in_column_const_iterator(const filled_tableau<T>::in_column_iterator& other)
		: tab(other.tab), column_number(other.column_number), row_number(other.row_number)
	{
	}

	template<class T>
	typename filled_tableau<T>::in_column_const_iterator filled_tableau<T>::in_column_const_iterator::operator+(unsigned int n) const
	{
		typename filled_tableau<T>::in_column_const_iterator it2(*this);
		it2 += n;
		return it2;
	}

	template<class T>
	typename filled_tableau<T>::in_column_const_iterator filled_tableau<T>::in_column_const_iterator::operator-(unsigned int n) const
	{
		typename filled_tableau<T>::in_column_const_iterator it2(*this);
		it2 -= n;
		return it2;
	}

	template<class T>
	ptrdiff_t filled_tableau<T>::in_column_const_iterator::operator-(const in_column_const_iterator& other) const
	{
		return row_number - other.row_number;
	}

	template<class T>
	const T& filled_tableau<T>::in_column_const_iterator::operator*() const
	{
		return (*tab)(row_number, column_number);
	}

	template<class T>
	const T* filled_tableau<T>::in_column_const_iterator::operator->() const
	{
		return &((*tab)(row_number, column_number));
	}

	template<class T>
	typename filled_tableau<T>::in_column_const_iterator& filled_tableau<T>::in_column_const_iterator::operator++()
	{
		++row_number;
		return (*this);
	}

	template<class T>
	typename filled_tableau<T>::in_column_const_iterator& filled_tableau<T>::in_column_const_iterator::operator+=(unsigned int n)
	{
		row_number += n;
		return (*this);
	}

	template<class T>
	typename filled_tableau<T>::in_column_const_iterator& filled_tableau<T>::in_column_const_iterator::operator--()
	{
		--row_number;
		return (*this);
	}

	template<class T>
	typename filled_tableau<T>::in_column_const_iterator filled_tableau<T>::in_column_const_iterator::operator--(int)
	{
		in_column_const_iterator tmp(*this);
		--row_number;
		return tmp;
	}

	template<class T>
	typename filled_tableau<T>::in_column_const_iterator filled_tableau<T>::in_column_const_iterator::operator++(int)
	{
		in_column_const_iterator tmp(*this);
		++row_number;
		return tmp;
	}

	template<class T>
	typename filled_tableau<T>::in_column_const_iterator& filled_tableau<T>::in_column_const_iterator::operator-=(unsigned int n)
	{
		row_number -= n;
		return (*this);
	}

	template<class T>
	bool filled_tableau<T>::in_column_const_iterator::operator==(const in_column_const_iterator& other) const
	{
		if (tab == other.tab && row_number == other.row_number && column_number == other.column_number)
			return true;
		return false;
	}

	template<class T>
	bool filled_tableau<T>::in_column_const_iterator::operator<=(const in_column_const_iterator & other) const
	{
		if (row_number <= other.row_number) return true;
		return false;
	}

	template<class T>
	bool filled_tableau<T>::in_column_const_iterator::operator>=(const in_column_const_iterator & other) const
	{
		if (row_number >= other.row_number) return true;
		return false;
	}

	template<class T>
	bool filled_tableau<T>::in_column_const_iterator::operator<(const in_column_const_iterator & other) const
	{
		if (row_number < other.row_number) return true;
		return false;
	}

	template<class T>
	bool filled_tableau<T>::in_column_const_iterator::operator>(const in_column_const_iterator & other) const
	{
		if (row_number > other.row_number) return true;
		return false;
	}

	template<class T>
	bool filled_tableau<T>::in_column_const_iterator::operator!=(const in_column_const_iterator & other) const
	{
		return !((*this) == other);
	}


	//---------------------------------------------------------------------------
	// in_row_iterator

	template<class T>
	filled_tableau<T>::in_row_iterator::in_row_iterator(unsigned int r, unsigned int c, filled_tableau<T>* t)
		: tab(t), column_number(c), row_number(r)
	{
	}

	template<class T>
	typename filled_tableau<T>::in_row_iterator filled_tableau<T>::in_row_iterator::operator+(unsigned int n) const
	{
		typename filled_tableau<T>::in_row_iterator it2(*this);
		it2 += n;
		return it2;
	}

	template<class T>
	typename filled_tableau<T>::in_row_iterator filled_tableau<T>::in_row_iterator::operator-(unsigned int n) const
	{
		typename filled_tableau<T>::in_row_iterator it2(*this);
		it2 -= n;
		return it2;
	}

	template<class T>
	ptrdiff_t filled_tableau<T>::in_row_iterator::operator-(const in_row_iterator& other) const
	{
		return column_number - other.column_number;
	}

	template<class T>
	T& filled_tableau<T>::in_row_iterator::operator*() const
	{
		return (*tab)(row_number, column_number);
	}

	template<class T>
	T* filled_tableau<T>::in_row_iterator::operator->() const
	{
		return &((*tab)(row_number, column_number));
	}

	template<class T>
	typename filled_tableau<T>::in_row_iterator& filled_tableau<T>::in_row_iterator::operator++()
	{
		++column_number;
		return (*this);
	}

	template<class T>
	typename filled_tableau<T>::in_row_iterator& filled_tableau<T>::in_row_iterator::operator+=(unsigned int n)
	{
		column_number += n;
		return (*this);
	}

	template<class T>
	typename filled_tableau<T>::in_row_iterator& filled_tableau<T>::in_row_iterator::operator--()
	{
		--column_number;
		return (*this);
	}

	template<class T>
	typename filled_tableau<T>::in_row_iterator filled_tableau<T>::in_row_iterator::operator--(int)
	{
		in_row_iterator tmp(*this);
		--column_number;
		return tmp;
	}

	template<class T>
	typename filled_tableau<T>::in_row_iterator filled_tableau<T>::in_row_iterator::operator++(int)
	{
		in_row_iterator tmp(*this);
		++column_number;
		return tmp;
	}

	template<class T>
	typename filled_tableau<T>::in_row_iterator& filled_tableau<T>::in_row_iterator::operator-=(unsigned int n)
	{
		column_number -= n;
		return (*this);
	}

	template<class T>
	bool filled_tableau<T>::in_row_iterator::operator==(const in_row_iterator& other) const
	{
		if (tab == other.tab && row_number == other.row_number && column_number == other.column_number)
			return true;
		return false;
	}

	template<class T>
	bool filled_tableau<T>::in_row_iterator::operator<=(const in_row_iterator & other) const
	{
		if (column_number <= other.column_number) return true;
		return false;
	}

	template<class T>
	bool filled_tableau<T>::in_row_iterator::operator>=(const in_row_iterator & other) const
	{
		if (column_number >= other.column_number) return true;
		return false;
	}

	template<class T>
	bool filled_tableau<T>::in_row_iterator::operator<(const in_row_iterator & other) const
	{
		if (column_number < other.column_number) return true;
		return false;
	}

	template<class T>
	bool filled_tableau<T>::in_row_iterator::operator>(const in_row_iterator & other) const
	{
		if (column_number > other.column_number) return true;
		return false;
	}

	template<class T>
	bool filled_tableau<T>::in_row_iterator::operator!=(const in_row_iterator & other) const
	{
		return !((*this) == other);
	}

	//---------------------------------------------------------------------------
// in_row_const_iterator

	template<class T>
	filled_tableau<T>::in_row_const_iterator::in_row_const_iterator(unsigned int r, unsigned int c, const filled_tableau<T>* t)
		: tab(t), column_number(c), row_number(r)
	{
	}

	template<class T>
	filled_tableau<T>::in_row_const_iterator::in_row_const_iterator(const filled_tableau<T>::in_row_iterator& other)
		: tab(other.tab), column_number(other.column_number), row_number(other.row_number)
	{
	}


	template<class T>
	typename filled_tableau<T>::in_row_const_iterator filled_tableau<T>::in_row_const_iterator::operator+(unsigned int n) const
	{
		typename filled_tableau<T>::in_row_const_iterator it2(*this);
		it2 += n;
		return it2;
	}

	template<class T>
	typename filled_tableau<T>::in_row_const_iterator filled_tableau<T>::in_row_const_iterator::operator-(unsigned int n) const
	{
		typename filled_tableau<T>::in_row_const_iterator it2(*this);
		it2 -= n;
		return it2;
	}

	template<class T>
	ptrdiff_t filled_tableau<T>::in_row_const_iterator::operator-(const in_row_const_iterator& other) const
	{
		return column_number - other.column_number;
	}

	template<class T>
	const T& filled_tableau<T>::in_row_const_iterator::operator*() const
	{
		return (*tab)(row_number, column_number);
	}

	template<class T>
	const T* filled_tableau<T>::in_row_const_iterator::operator->() const
	{
		return &((*tab)(row_number, column_number));
	}

	template<class T>
	typename filled_tableau<T>::in_row_const_iterator& filled_tableau<T>::in_row_const_iterator::operator++()
	{
		++column_number;
		return (*this);
	}

	template<class T>
	typename filled_tableau<T>::in_row_const_iterator& filled_tableau<T>::in_row_const_iterator::operator+=(unsigned int n)
	{
		column_number += n;
		return (*this);
	}

	template<class T>
	typename filled_tableau<T>::in_row_const_iterator& filled_tableau<T>::in_row_const_iterator::operator--()
	{
		--column_number;
		return (*this);
	}

	template<class T>
	typename filled_tableau<T>::in_row_const_iterator filled_tableau<T>::in_row_const_iterator::operator--(int)
	{
		in_row_const_iterator tmp(*this);
		--column_number;
		return tmp;
	}

	template<class T>
	typename filled_tableau<T>::in_row_const_iterator filled_tableau<T>::in_row_const_iterator::operator++(int)
	{
		in_row_const_iterator tmp(*this);
		++column_number;
		return tmp;
	}

	template<class T>
	typename filled_tableau<T>::in_row_const_iterator& filled_tableau<T>::in_row_const_iterator::operator-=(unsigned int n)
	{
		column_number -= n;
		return (*this);
	}

	template<class T>
	bool filled_tableau<T>::in_row_const_iterator::operator==(const in_row_const_iterator& other) const
	{
		if (tab == other.tab && row_number == other.row_number && column_number == other.column_number)
			return true;
		return false;
	}

	template<class T>
	bool filled_tableau<T>::in_row_const_iterator::operator<=(const in_row_const_iterator & other) const
	{
		if (column_number <= other.column_number) return true;
		return false;
	}

	template<class T>
	bool filled_tableau<T>::in_row_const_iterator::operator>=(const in_row_const_iterator & other) const
	{
		if (column_number >= other.column_number) return true;
		return false;
	}

	template<class T>
	bool filled_tableau<T>::in_row_const_iterator::operator<(const in_row_const_iterator & other) const
	{
		if (column_number < other.column_number) return true;
		return false;
	}

	template<class T>
	bool filled_tableau<T>::in_row_const_iterator::operator>(const in_row_const_iterator & other) const
	{
		if (column_number > other.column_number) return true;
		return false;
	}

	template<class T>
	bool filled_tableau<T>::in_row_const_iterator::operator!=(const in_row_const_iterator & other) const
	{
		return !((*this) == other);
	}



	//---------------------------------------------------------------------------
	// iterator

	template<class T>
	filled_tableau<T>::iterator::iterator(unsigned int r, unsigned int c, filled_tableau<T> *t)
		: tab(t), column_number(c), row_number(r)
		{
		}

	template<class T>
	typename filled_tableau<T>::iterator filled_tableau<T>::iterator::operator+(unsigned int n) const
		{
		typename filled_tableau<T>::iterator it2(*this);
		it2+=n;
		return it2;
		}

	template<class T>
	typename filled_tableau<T>::iterator filled_tableau<T>::iterator::operator-(unsigned int n) const
		{
		typename filled_tableau<T>::iterator it2(*this);
		it2-=n;
		return it2;
		}

	template<class T>
	ptrdiff_t filled_tableau<T>::iterator::operator-(const iterator& other) const
		{
		return row_number-other.row_number;
		}

	template<class T>
	T& filled_tableau<T>::iterator::operator*() const
		{
		return (*tab)(row_number,column_number);
		}

	template<class T>
	T* filled_tableau<T>::iterator::operator->() const
		{
		return &((*tab)(row_number,column_number));
		}

	template<class T>
	typename filled_tableau<T>::iterator& filled_tableau<T>::iterator::operator++()
		{
		if(++column_number==tab->rows[row_number].size()) {
			column_number=0;
			++row_number;
			}
		return (*this);
		}

	template<class T>
	typename filled_tableau<T>::iterator& filled_tableau<T>::iterator::operator+=(unsigned int n)
		{
		while(n>0) {
			if(++column_number==tab->rows[row_number]) {
				column_number=0;
				++row_number;
				}
			--n;
			}
		return (*this);
		}

	template<class T>
	typename filled_tableau<T>::iterator& filled_tableau<T>::iterator::operator--()
		{
		if(column_number==0) {
			--row_number;
			column_number=tab->rows[row_number].size()-1;
			}
		else --column_number;
		return (*this);
		}

	template<class T>
	typename filled_tableau<T>::iterator filled_tableau<T>::iterator::operator--(int)
		{
		iterator tmp(*this);
		if(column_number==0) {
			--row_number;
			column_number=tab->rows[row_number].size()-1;
			}
		else --column_number;
		return tmp;
		}

	template<class T>
	typename filled_tableau<T>::iterator filled_tableau<T>::iterator::operator++(int)
		{
		iterator tmp(*this);
		while(this->n>0) {
			if(++column_number==tab->rows[row_number]) {
				column_number=0;
				++row_number;
				}
			--this->n;
			}
		return tmp;
		}

	template<class T>
	typename filled_tableau<T>::iterator& filled_tableau<T>::iterator::operator-=(unsigned int n)
		{
		while(n>0) {
			if(column_number==0) {
				--row_number;
				column_number=tab->rows[row_number].size()-1;
				}
			else --column_number;
			--n;
			}
		return (*this);
		}

	template<class T>
	bool filled_tableau<T>::iterator::operator==(const iterator& other) const
		{
		if(tab==other.tab && row_number==other.row_number && column_number==other.column_number)
			return true;
		return false;
		}

	template<class T>
	bool filled_tableau<T>::iterator::operator<(const iterator& other) const
		{
		if(row_number<other.row_number) return true;
		return false;
		}

	template<class T>
	bool filled_tableau<T>::iterator::operator>(const iterator& other) const
		{
		if(row_number>other.row_number) return true;
		return false;
		}

	template<class T>
	bool filled_tableau<T>::iterator::operator!=(const iterator& other) const
		{
		return !((*this)==other);
		}



	//---------------------------------------------------------------------------
	// const_iterator

	template<class T>
	filled_tableau<T>::const_iterator::const_iterator(unsigned int r, unsigned int c, const filled_tableau<T>* t)
		: tab(t), column_number(c), row_number(r)
	{
	}

	template<class T>
	filled_tableau<T>::const_iterator::const_iterator(const filled_tableau<T>::iterator& other)
		: tab(other.tab), column_number(other.column_number), row_number(other.row_number)
	{
	}

	template<class T>
	typename filled_tableau<T>::const_iterator filled_tableau<T>::const_iterator::operator+(unsigned int n) const
	{
		typename filled_tableau<T>::const_iterator it2(*this);
		it2 += n;
		return it2;
	}

	template<class T>
	typename filled_tableau<T>::const_iterator filled_tableau<T>::const_iterator::operator-(unsigned int n) const
	{
		typename filled_tableau<T>::const_iterator it2(*this);
		it2 -= n;
		return it2;
	}

	template<class T>
	ptrdiff_t filled_tableau<T>::const_iterator::operator-(const const_iterator& other) const
	{
		return row_number - other.row_number;
	}

	template<class T>
	const T& filled_tableau<T>::const_iterator::operator*() const
	{
		return (*tab)(row_number, column_number);
	}

	template<class T>
	const T* filled_tableau<T>::const_iterator::operator->() const
	{
		return &((*tab)(row_number, column_number));
	}

	template<class T>
	typename filled_tableau<T>::const_iterator& filled_tableau<T>::const_iterator::operator++()
	{
		if (++column_number == tab->rows[row_number].size()) {
			column_number = 0;
			++row_number;
		}
		return (*this);
	}

	template<class T>
	typename filled_tableau<T>::const_iterator& filled_tableau<T>::const_iterator::operator+=(unsigned int n)
	{
		while (n > 0) {
			if (++column_number == tab->rows[row_number]) {
				column_number = 0;
				++row_number;
			}
			--n;
		}
		return (*this);
	}

	template<class T>
	typename filled_tableau<T>::const_iterator& filled_tableau<T>::const_iterator::operator--()
	{
		if (column_number == 0) {
			--row_number;
			column_number = tab->rows[row_number].size() - 1;
		}
		else --column_number;
		return (*this);
	}

	template<class T>
	typename filled_tableau<T>::const_iterator filled_tableau<T>::const_iterator::operator--(int)
	{
		const_iterator tmp(*this);
		if (column_number == 0) {
			--row_number;
			column_number = tab->rows[row_number].size() - 1;
		}
		else --column_number;
		return tmp;
	}

	template<class T>
	typename filled_tableau<T>::const_iterator filled_tableau<T>::const_iterator::operator++(int)
	{
		const_iterator tmp(*this);
		while (this->n > 0) {
			if (++column_number == tab->rows[row_number]) {
				column_number = 0;
				++row_number;
			}
			--this->n;
		}
		return tmp;
	}

	template<class T>
	typename filled_tableau<T>::const_iterator& filled_tableau<T>::const_iterator::operator-=(unsigned int n)
	{
		while (n > 0) {
			if (column_number == 0) {
				--row_number;
				column_number = tab->rows[row_number].size() - 1;
			}
			else --column_number;
			--n;
		}
		return (*this);
	}

	template<class T>
	bool filled_tableau<T>::const_iterator::operator==(const const_iterator& other) const
	{
		if (tab == other.tab && row_number == other.row_number && column_number == other.column_number)
			return true;
		return false;
	}

	template<class T>
	bool filled_tableau<T>::const_iterator::operator<(const const_iterator & other) const
	{
		if (row_number < other.row_number) return true;
		return false;
	}

	template<class T>
	bool filled_tableau<T>::const_iterator::operator>(const const_iterator & other) const
	{
		if (row_number > other.row_number) return true;
		return false;
	}

	template<class T>
	bool filled_tableau<T>::const_iterator::operator!=(const const_iterator & other) const
	{
		return !((*this) == other);
	}


	//---
	// other

	template<class T>
	typename filled_tableau<T>::iterator filled_tableau<T>::begin()
	{
		return iterator(0, 0, this);
	}

	template<class T>
	typename filled_tableau<T>::iterator filled_tableau<T>::end()
	{
		return iterator(rows.size(), 0, this);
	}


	template<class T>
	typename filled_tableau<T>::const_iterator filled_tableau<T>::cbegin() const
		{
		return const_iterator(0,0,this);
		}

	template<class T>
	typename filled_tableau<T>::const_iterator filled_tableau<T>::cend() const
		{
		return const_iterator(rows.size(), 0, this);
		}

	template<class T>
	typename filled_tableau<T>::const_iterator filled_tableau<T>::begin() const
	{
		return cbegin();
	}

	template<class T>
	typename filled_tableau<T>::const_iterator filled_tableau<T>::end() const
	{
		return cend();
	}

	template<class T>
	typename filled_tableau<T>::in_column_iterator filled_tableau<T>::begin_column(unsigned int column)
		{
		typename filled_tableau<T>::in_column_iterator it(0,column,this);
		assert(number_of_rows()>0);
		assert(column<row_size(0));
		return it;
		}

	template<class T>
	typename filled_tableau<T>::in_column_iterator filled_tableau<T>::end_column(unsigned int column)
		{
		unsigned int r=0;
		while(r<number_of_rows()) {
			if(row_size(r)<=column)
				break;
			++r;
			}
		typename filled_tableau<T>::in_column_iterator it(r,column,this);
		return it;
		}

	template<class T>
	typename filled_tableau<T>::in_column_const_iterator filled_tableau<T>::cbegin_column(unsigned int column) const
	{
		typename filled_tableau<T>::in_column_const_iterator it(0, column, this);
		assert(number_of_rows() > 0);
		assert(column < row_size(0));
		return it;
	}

	template<class T>
	typename filled_tableau<T>::in_column_const_iterator filled_tableau<T>::cend_column(unsigned int column) const
	{
		unsigned int r = 0;
		while (r < number_of_rows()) {
			if (row_size(r) <= column)
				break;
			++r;
		}
		typename filled_tableau<T>::in_column_const_iterator it(r, column, this);
		return it;
	}

	template<class T>
	typename filled_tableau<T>::in_column_const_iterator filled_tableau<T>::begin_column(unsigned int column) const
	{
		return cbegin_column(column);
	}

	template<class T>
	typename filled_tableau<T>::in_column_const_iterator filled_tableau<T>::end_column(unsigned int column) const
	{
		return cend_column(column);
	}

	template<class T>
	typename filled_tableau<T>::in_row_iterator filled_tableau<T>::begin_row(unsigned int row)
	{
		return in_row_iterator{ row, 0, this };
	}

	template<class T>
	typename filled_tableau<T>::in_row_iterator filled_tableau<T>::end_row(unsigned int row)
	{
		return in_row_iterator{ row, row_size(row), this };
	}

	template<class T>
	typename filled_tableau<T>::in_row_const_iterator filled_tableau<T>::cbegin_row(unsigned int row) const
	{
		return in_row_const_iterator{ row, 0, this };
	}

	template<class T>
	typename filled_tableau<T>::in_row_const_iterator filled_tableau<T>::cend_row(unsigned int row) const
	{
		return in_row_const_iterator{ row, row_size(row), this };
	}

	template<class T>
	typename filled_tableau<T>::in_row_const_iterator filled_tableau<T>::begin_row(unsigned int row) const
	{
		return cbegin_row(row);
	}

	template<class T>
	typename filled_tableau<T>::in_row_const_iterator filled_tableau<T>::end_row(unsigned int row) const
	{
		return cend_row(row);
	}




	template<class T>
	template<class OutputIterator>
	OutputIterator filled_tableau<T>::Garnir_set(OutputIterator it, unsigned int row, unsigned int col) const
		{
		assert(col>0);
		unsigned int r=row, c=col;
		*it=(*this)(r,c);
		++it;
		while(r>0) {
			--r;
			*it=(*this)(r,c);
			++it;
			}
		r=row;
		--c;
		*it=(*this)(r,c);
		++it;
		while(r+1<column_size(c)) {
			++r;
			*it=(*this)(r,c);
			++it;
			}
		return it;
		}

	template<class T>
	std::pair<int, int> filled_tableau<T>::nonstandard_loc() const
		{
		unsigned int r=number_of_rows();
		assert(r>0);
		do {
			--r;
			for(unsigned int c=0; c<row_size(r)-1; ++c) {
				if((*this)(r,c) > (*this)(r,c+1) )
					return std::pair<int, int>(r,c);
				}
			}
		while(r>0);
		return std::pair<int,int>(-1,-1);
		}

	template<class T>
	bool tableaux<T>::standard_form()
		{
		bool already_standard=true;

		typename tableau_container_t::iterator thetab=storage.begin();
		while(thetab!=storage.end()) {
			(*thetab).sort_within_columns();
			std::pair<int,int> where=(*thetab).nonstandard_loc();
			if(where.first!=-1) {
				already_standard=false;
				combin::combinations<typename T::value_type> com;
				for(unsigned int i1=where.first; i1<(*thetab).column_size(where.second); ++i1)
					com.original.push_back((*thetab)(i1,where.second));
				for(unsigned int i1=0; i1<=(unsigned int)(where.first); ++i1)
					com.original.push_back((*thetab)(i1,where.second+1));
				com.sublengths.push_back((*thetab).column_size(where.second)-where.first);
				com.sublengths.push_back(where.first+1);
				com.permute();
				for(unsigned int tabi=1; tabi<com.size(); ++tabi) {
					T ntab((*thetab));
					unsigned int offset=0;
					for(unsigned int i1=where.first; i1<(*thetab).column_size(where.second); ++i1, ++offset)
						ntab(i1,where.second)=com[tabi][offset];
					for(unsigned int i1=0; i1<=(unsigned int)(where.first); ++i1, ++offset)
						ntab(i1,where.second+1)=com[tabi][offset];
					ntab.multiplicity*=-1*com.ordersign(tabi);
					add_tableau(ntab);
					}
				thetab=storage.erase(thetab);
				}
			else ++thetab;
			}
		return already_standard;
		}

	template<class T>
	void tableaux<T>::add_tableau(const T& ntab)
		{
		typename tableau_container_t::iterator it=storage.begin();
		while(it!=storage.end()) {
			if((*it).compare_without_multiplicity(ntab)) {
				(*it).multiplicity+=ntab.multiplicity;
				if((*it).multiplicity==0)
					storage.erase(it);
				return;
				}
			++it;
			}
		storage.push_back(ntab);
		}


	template<class T>
	yngrat_t filled_tableau<T>::projector_normalisation() const
		{
		yngrat_t norm=1;
		norm/=hook_length_prod();
		return norm;
		}

	template<class T>
	void filled_tableau<T>::projector(combin::symmetriser<T>& sym, bool modulo_monoterm) const
		{
		for(unsigned int r=0; r<number_of_rows(); ++r)
			for(unsigned int c=0; c<row_size(r); ++c)
				sym.original.push_back(rows[r][c]);

		unsigned int offset=0;
		// symmetrise over boxes in rows
		for(unsigned int r=0; r<number_of_rows(); ++r) {
			sym.permutation_sign=1;
			sym.permute_blocks.clear();
			sym.block_length=1;
			sym.input_asym.clear();
			for(unsigned int c=0; c<row_size(r); ++c)
				sym.permute_blocks.push_back(offset++);
			sym.apply_symmetry();
			}
		//	sym.collect();
		// anti-symmetrise over boxes in columns
		if(modulo_monoterm) {
			int newmult=1;
			for(unsigned int c=0; c<row_size(0); ++c)
				newmult*=combin::factorial(column_size(c));
			for(unsigned int i=0; i<sym.size(); ++i)
				sym.set_multiplicity(i, sym.signature(i)*newmult);
			}
		else {
			sym.permute_blocks.clear();
			for(unsigned int c=0; c<row_size(0); ++c) {
				unsigned int r=0;
				sym.value_permute.clear();
				sym.permutation_sign=-1;
				sym.input_asym.clear();
				while(r<number_of_rows() && c<row_size(r))
					sym.value_permute.push_back(rows[r++][c]);
				if(sym.value_permute.size()>1)
					sym.apply_symmetry();
				}
			}
		//	sym.collect();
		}

	template<class T>
	void filled_tableau<T>::projector(combin::symmetriser<T>& sym,
	                                  combin::range_vector_t& sublengths_scattered) const
		{
		for(unsigned int r=0; r<number_of_rows(); ++r)
			for(unsigned int c=0; c<row_size(r); ++c)
				sym.original.push_back(rows[r][c]);

		unsigned int offset=0;
		// symmetrise over boxes in rows
		for(unsigned int r=0; r<number_of_rows(); ++r) {
			sym.permutation_sign=1;
			sym.permute_blocks.clear();
			sym.block_length=1;
			sym.input_asym.clear();
			for(unsigned int c=0; c<row_size(r); ++c)
				sym.permute_blocks.push_back(offset++);
			sym.apply_symmetry();
			}
		/// anti-symmetrise over boxes in columns
		sym.permute_blocks.clear();
		for(unsigned int c=0; c<row_size(0); ++c) {
			unsigned int r=0;
			sym.value_permute.clear();
			sym.permutation_sign=-1;
			while(r<number_of_rows() && c<row_size(r))
				sym.value_permute.push_back(rows[r++][c]);

			sym.sublengths_scattered=sublengths_scattered;

			//		// Now setup sublengths_scattered to take into account
			//		// asym_ranges.  These asym_ranges refer to values stored in the
			//		// boxes of the full tableau. We need to find the locations of
			//		// these values inside the full original, as that is what goes
			//		// into sublengths_scattered.
			//
			//		sym.input_asym.clear();
			//		sym.sublengths.clear();
			//		sym.sublengths_scattered.clear();
			//		for(unsigned int m=0; m<sym.value_permute.size(); ++m) {
			//			// Try to find this value in an asym range.
			//			unsigned int overlap=0;
			//			for(unsigned int n=0; n<asym_ranges.size(); ++n) {
			//				for(unsigned int nn=0; nn<asym_ranges[n].size(); ++nn) {
			//					if(sym.value_permute[m]==asym_ranges[n][nn]) {
			//						std::cout << "found " << sym.value_permute[m] << " in range" << std::endl;
			//						// FIXME: this assumes that even though asym_ranges[n] is a superset
			//						// of the current part of value_permute, elements are in the same order.
			//						++m; ++nn;
			//						while(nn<asym_ranges[n].size()) {
			//							if(sym.value_permute[m]==asym_ranges[n][nn]) {
			//								std::cout << "same range: " << sym.value_permute[m] << std::endl;
			//								++m;
			//								++overlap;
			//								}
			//							++nn;
			//							}
			//						break;
			//						}
			//					}
			//				}
			//			if(overlap>0) --m;
			//			sym.sublengths.push_back(overlap+1);
			//			}
			//		unsigned int sum=0;
			//		for(unsigned int m=0; m<sym.sublengths.size(); ++m)
			//			sum+=sym.sublengths[m];
			//
			//		std::cout << sum << " " << sym.value_permute.size() << std::endl;
			//		assert(sum==sym.value_permute.size());

			// All set to run...
			if(sym.value_permute.size()>1)
				sym.apply_symmetry();
			}
		}

	template<class T>
	filled_tableau<T>::filled_tableau()
		: tableau()
		{
		}

	template<class T>
	filled_tableau<T>::filled_tableau(const filled_tableau<T>& other)
		: tableau(other), rows(other.rows)
		{
		}

	template<class T>
	filled_tableau<T>& filled_tableau<T>::operator=(const filled_tableau<T>& other)
		{
		rows=other.rows;
		tableau::operator=(other);
		return (*this);
		}

	template<class T>
	yngint_t tableaux<T>::total_dimension(unsigned int dim)
		{
		yngint_t totdim=0;
		typename tableau_container_t::const_iterator it=storage.begin();
		while(it!=storage.end()) {
			totdim+=(*it).dimension(dim);
			++it;
			}
		return totdim;
		}

	template<class T, class OutputIterator>
	void LR_tensor(const tableaux<T>& tabs1, const T& tab2, unsigned int maxrows,
	               OutputIterator out, bool alltabs=false)
		{
		typename tableaux<T>::tableau_container_t::const_iterator it=tabs1.storage.begin();
		while(it!=tabs1.storage.end()) {
			LR_tensor((*it), tab2, maxrows, out, alltabs);
			++it;
			}
		}

	template<class T1, class T2>
	void add_box(T1& tab1, unsigned int row1,
	             const T2& tab2, unsigned int row2, unsigned int col2)
		{
		tab1.add_box(row1, tab2(row2,col2));
		}

	template<class T1>
	void add_box(T1& tab1, unsigned int row1,
	             const tableau&, unsigned int, unsigned int)
		{
		tab1.add_box(row1);
		}

	typedef filled_tableau<std::pair<int, int> > keeptrack_tab_t;

	template<class Tab, class OutputIterator>
	void LR_add_box(const Tab& tab2, Tab& newtab,
	                unsigned int currow2, unsigned int curcol2, unsigned int startrow,
	                unsigned int maxrows,
	                OutputIterator outit,
	                keeptrack_tab_t& Ycurrent,
	                bool alltabs)
		{
		// Are we at the end of the current row of boxes in tab2 ?
		if((++curcol2)==tab2.row_size(currow2)) {
			// Are we at the end of tab2 altogether?
			if((++currow2)==tab2.number_of_rows()) {
				*outit=newtab;  // Store the product tableau just created.
				return;
				}
			curcol2=0;
			startrow=0;
			}

		// Rule "row_by_row".
		for(unsigned int rowpos=startrow; rowpos<std::min(newtab.number_of_rows()+1,maxrows); ++rowpos) {
			// Rule "always_young".
			if(rowpos>0 && rowpos<newtab.number_of_rows())
				if(newtab.row_size(rowpos-1)==newtab.row_size(rowpos))
					continue; // No, would lead to non-Young tableau shape.

			// The column where the box will be added.
			unsigned int colpos=(rowpos==newtab.number_of_rows())?0:newtab.row_size(rowpos);

			// Rule "avoid_sym2asym".
			for(unsigned int rr=0; rr<rowpos; ++rr)
				if(Ycurrent(rr,colpos).first==(int)(currow2))
					goto rule_violated;

			// Rule "avoid_asym2sym".
			if(alltabs) // if not generating all tabs, ordered will take care of this already.
				for(unsigned int cc=0; cc<colpos; ++cc)
					if(Ycurrent(rowpos,cc).second==(int)(curcol2))
						goto rule_violated;

			// Rule "ordered".
			if(!alltabs && currow2>0) {
				int numi=0, numimin1=0;
				if(rowpos>0) {
					for(unsigned int sr=0; sr<rowpos; ++sr)  // top to bottom
						for(unsigned int sc=0; sc<Ycurrent.row_size(sr); ++sc) { // right to left
							// Count all boxes from currow2 and from currow2-1.
							if(Ycurrent(sr,sc).first==(int)(currow2))    ++numi;
							if(Ycurrent(sr,sc).first==(int)(currow2)-1)  ++numimin1;
							}
					}
				++numi; // the box to be added
				if(numi>numimin1)
					goto rule_violated;

				// continue counting to see whether a previously valid box is now invalid
				for(unsigned int sr=rowpos; sr<Ycurrent.number_of_rows(); ++sr)  // top to bottom
					for(int sc=Ycurrent.row_size(sr)-1; sc>=0; --sc) { // right to left
						if(Ycurrent(sr,sc).first==(int)(currow2))    ++numi;
						if(Ycurrent(sr,sc).first==(int)(currow2)-1)  ++numimin1;
						if(numi>numimin1)
							goto rule_violated;
						}
				}

			// Put the box at row 'rowpos' and call LR_add_box recursively
			// to add the other boxes.
			Ycurrent.add_box(rowpos, std::pair<int,int>(currow2, curcol2));
			add_box(newtab, rowpos, tab2, currow2, curcol2);
			LR_add_box(tab2, newtab, currow2, curcol2, alltabs?0:rowpos, maxrows,
			           outit, Ycurrent, alltabs);

			// Remove the box again in preparation for trying to add it to other rows.
			newtab.remove_box(rowpos);
			Ycurrent.remove_box(rowpos);

rule_violated:
			;
			}
		}

	template<class Tab, class OutputIterator>
	void LR_tensor(const Tab& tab1, const Tab& tab2, unsigned int maxrows,
	               OutputIterator outit, bool alltabs=false)
		{
		// Make a copy of tab1 because LR_add_box has to change it and
		// tab1 is const here.
		Tab newtab(tab1);

		// Tableau which keeps track of the LR rules. It contains the
		// current (incomplete) shape of the tensor product, and for all boxes
		// which come from tab2 we store the row and column of tab2
		// from which they originated. Tab1 boxes have (-2,-2) stored.
		keeptrack_tab_t Ycurrent;
		Ycurrent.copy_shape(tab1);
		keeptrack_tab_t::iterator yi=Ycurrent.begin();
		while(yi!=Ycurrent.end()) {
			(*yi)=std::pair<int,int>(-2,-2);
			++yi;
			}

		LR_add_box(tab2, newtab, 0, -1, 0, maxrows, outit, Ycurrent, alltabs);
		}

	template<class T, class OutputIterator>
	void LR_tensor(const tableaux<T>&, bool, unsigned int, OutputIterator )
		{
		assert(1==0);
		}



	std::ostream& operator<<(std::ostream&, const tableau& );

	template<class T>
	std::ostream& operator<<(std::ostream&, const tableaux<T>& );

	template<class T>
	std::ostream& operator<<(std::ostream&, const filled_tableau<T>& );

	template<class T>
	unsigned int filled_tableau<T>::number_of_rows() const
		{
		return rows.size();
		}

	template<class T>
	unsigned int filled_tableau<T>::row_size(unsigned int num) const
		{
		assert(num<rows.size());
		return rows[num].size();
		}

	template<class T>
	T& filled_tableau<T>::operator()(unsigned int row, unsigned int col)
		{
		assert(row<rows.size());
		assert(col<rows[row].size());
		return rows[row][col];
		}

	template<class T>
	const T& filled_tableau<T>::operator()(unsigned int row, unsigned int col)  const
		{
		assert(row<rows.size());
		assert(col<rows[row].size());
		return rows[row][col];
		}

	template<class T>
	const T& filled_tableau<T>::operator[](unsigned int boxnum) const
		{
		unsigned int row = 0;
		while (true) {
			if (boxnum < row_size(row))
				break;
			boxnum -= row_size(row);
			++row;
		}
		return rows[row][boxnum];
		}

	template<class T>
	filled_tableau<T>::~filled_tableau()
		{
		}

	template<class T>
	void filled_tableau<T>::add_box(unsigned int rownum)
		{
		if(rownum>=rows.size())
			rows.resize(rownum+1);
		assert(rownum<rows.size());
		rows[rownum].push_back(T());
		}

	template<class T>
	void filled_tableau<T>::add_box(unsigned int rownum, T val)
		{
		if(rownum>=rows.size())
			rows.resize(rownum+1);
		assert(rownum<rows.size());
		rows[rownum].push_back(val);
		}

	template<class T>
	void filled_tableau<T>::swap_columns(unsigned int c1, unsigned int c2)
		{
		assert(c1<row_size(0) && c2<row_size(0));
		assert(column_size(c1)==column_size(c2));
		for(unsigned int r=0; r<column_size(c1); ++r) {
			T tmp=(*this)(r,c1);
			(*this)(r,c1)=(*this)(r,c2);
			(*this)(r,c2)=tmp;
			}
		}

	template<class T>
	void filled_tableau<T>::remove_box(unsigned int rownum)
		{
		assert(rownum<rows.size());
		assert(rows[rownum].size()>0);
		rows[rownum].pop_back();
		if(rows[rownum].size()==0)
			rows.pop_back();
		}

	template<class T>
	void filled_tableau<T>::clear()
		{
		rows.clear();
		tableau::clear();
		}

	template<class T>
	std::ostream& operator<<(std::ostream& str, const tableaux<T>& tabs)
		{
		typename tableaux<T>::tableau_container_t::const_iterator it=tabs.storage.begin();
		while(it!=tabs.storage.end()) {
			str << (*it) << std::endl << std::endl;
			++it;
			}
		return str;
		}

	template<class T>
	std::ostream& operator<<(std::ostream& str, const filled_tableau<T>& tab)
		{
		for(unsigned int i=0; i<tab.number_of_rows(); ++i) {
			for(unsigned int j=0; j<tab.row_size(i); ++j) {
				//			str << "|" << tab(i,j) << "|";
				str << tab(i,j);
				}
			if(i==0) {
				str << "  " << tab.dimension(10);
				if(tab.has_nullifying_trace()) str << " null";
				}
			if(i!=tab.number_of_rows()-1)
				str << std::endl;
			else
				str << " (" << tab.multiplicity << ")" << std::endl;
			}
		return str;
		}

	};


