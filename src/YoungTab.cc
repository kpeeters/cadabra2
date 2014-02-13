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

#include "YoungTab.hh"
#include <cassert>

namespace yngtab {

tableau_base::tableau_base()
	: multiplicity(1), selfdual_column(0)
	{
	}

tableau_base::~tableau_base()
	{
	}

tableau::~tableau()
	{
	}

void tableau_base::add_row(unsigned int row_size) 
	{
	assert(row_size>0);
	unsigned int row=number_of_rows();
	for(unsigned int i=0; i<row_size; ++i)
		add_box(row);
	}

yngint_t tableau_base::dimension(unsigned int dim) const
	{
	yngint_t ret=1;
	for(unsigned int r=0; r<number_of_rows(); ++r) {
		unsigned int backup=dim;
		for(unsigned int c=0; c<row_size(r); ++c) 
			ret*=(dim++);
		dim=backup-1;
		}
	assert(ret%hook_length_prod()==0);
	ret/=hook_length_prod();
	return ret;
	}

unsigned int tableau_base::column_size(unsigned int c) const
	{
	unsigned int r=0;
	while(r<number_of_rows()) {
		if(row_size(r)-1<c)
			break;
		++r;
		}
	return r;
	}

unsigned long tableau_base::hook_length(unsigned int row, unsigned int col) const
	{
	assert(row<number_of_rows());
	assert(col<row_size(row));
	unsigned long hook=row_size(row)-col;
	for(unsigned int i=row+1; i<number_of_rows() && col<row_size(i); ++i)
		++hook;
	return hook;
	}

yngint_t tableau_base::hook_length_prod() const
	{
	yngint_t hook=1;
	for(unsigned int i=0; i<number_of_rows(); ++i)
		for(unsigned int j=0; j<row_size(i); ++j)
			hook*=hook_length(i,j);
	return hook;
	}

void tableau::add_box(unsigned int rownum)
	{
	if(rownum>=rows.size()) {
		unsigned int prevsize=rows.size();
		rows.resize(rownum+1);
		for(unsigned int i=prevsize; i<rows.size(); ++i)
			rows[i]=0;
		}
	++rows[rownum];
	}

void tableau::remove_box(unsigned int rownum)
	{
	assert(rownum<rows.size());
	assert(rows[rownum]>0);
	if((--rows[rownum])==0)
		rows.pop_back();
	}

unsigned int tableau::number_of_rows() const
	{
	return rows.size();
	}

unsigned int tableau::row_size(unsigned int num) const
	{
	assert(num<rows.size());
	return rows[num];
	}

void tableau::clear() 
	{
	rows.clear();
	}

tableau& tableau::operator=(const tableau& other) 
	{
	rows=other.rows;
	return (*this);
	}

std::ostream& operator<<(std::ostream& str, const tableau& tab)
	{
	for(unsigned int i=0; i<tab.number_of_rows(); ++i) {
		for(unsigned int j=0; j<tab.row_size(i); ++j) {
//			str << "|x|";
			str << "x";
			}
		if(i==0) str << "  " << tab.dimension(10) << " " << tab.hook_length_prod();
		if(i!=tab.number_of_rows()-1)
			str << std::endl;
		}
	return str;
	}

//bool legal_box(const std::vector<std::pair<int,int> >& prev, 
//					const std::vector<std::pair<int,int> >& ths, 
//					int colpos, int rowpos)
//	{
//	// Note the initial condition: upon startup, the prev vector contains as many -1,-1
//   // pairs as boxes in the first row of tableau 2. So 'prevabove' will initially evaluate
//	// to this number.
//
//	int prevabove=0; // number of boxes of the previous row added above the candidate position
//	int thisabove=0; // number of boxes of the current  row added above the candidate position
//	for(unsigned int i=0; i<prev.size(); ++i) {
//		if(prev[i].second<rowpos)
//			++prevabove;
//		}
//	for(unsigned int i=0; i<ths.size(); ++i) {
//		if(ths[i].first==colpos) return false; // would imply anti-symmetrisation of symmetrised boxes
//		if(ths[i].second<rowpos)
//			++thisabove;
//		}
//	if(prevabove>thisabove) return true;
//	return false;
//	}

#ifndef __CYGWIN__
template<>
void add_box(tableau& tab1, unsigned int row1,
				 const tableau& tab2, unsigned int row2, unsigned int col2)
	{
	tab1.add_box(row1);
	}
#endif

};
