/* 

	Cadabra: a field-theory motivated computer algebra system.
	Copyright (C) 2001-2014  Kasper Peeters <kasper.peeters@phi-sci.com>

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

#include "Combinatorics.hh"

unsigned long combin::factorial(unsigned int x)
	{
	unsigned long ret=1;
	while(x) { ret*=x--; }
	return ret;
	}

int combin::determine_intersection_ranges(const combin::range_vector_t& prod,
											 const combin::range_vector_t& indv,
											 combin::range_vector_t& target)
	{
	int ret=1;
	for(unsigned int i=0; i<prod.size(); ++i) { // all ranges in prod
		for(unsigned int k=0; k<indv.size(); ++k) { // all ranges in indv
			combin::range_t newrange;
			for(unsigned int ic=0; ic<prod[i].size(); ++ic) { 
				for(unsigned int kc=0; kc<indv[k].size(); ++kc) {
					if(prod[i][ic] == indv[k][kc])
						newrange.push_back(indv[k][kc]);
					}
				}
			if(newrange.size()>=2) {
				ret*=factorial(newrange.size());
				target.push_back(newrange);
				}
			}
		}

	return ret;
	}

long combin::vector_sum(const std::vector<int>& v)
	{
	long ret=0;
	for(unsigned int i=0; i<v.size(); ++i)
		ret+=v[i];
	return ret;
	}

unsigned long combin::vector_prod(const std::vector<unsigned int>& v)
	{
	unsigned long ret=1;
	for(unsigned int i=0; i<v.size(); ++i)
		ret*=v[i];
	return ret;
	}

unsigned long combin::vector_prod_fact(const std::vector<unsigned int>& v)
	{
	unsigned long ret=1;
	for(unsigned int i=0; i<v.size(); ++i)
		ret*=fact(v[i]);
	return ret;
	}

bool combin::operator==(const std::vector<unsigned int>& one, const std::vector<unsigned int>& two)
	{
	if(one.size()!=two.size()) return false;

	for(unsigned int k=0; k<one.size(); ++k)
		if(one[k]!=two[k]) return false;
	
	return true;
	}

long combin::hash(const std::vector<unsigned int>& one)
	{
	long ret=1;
	for(unsigned int k=0; k<one.size(); ++k) {
		ret*=17;
		ret+=one[k];
		}
	return ret;
	}
