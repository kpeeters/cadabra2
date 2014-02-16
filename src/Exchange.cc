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

#include <map>

#include "Exchange.hh"
#include "Numerical.hh"
#include "properties/DiracBar.hh"

// Find groups of identical tensors. 
//
int exchange::collect_identical_tensors(exptree& tr, exptree::iterator it,
													  std::vector<identical_tensors_t>& idts) 
	{
	assert(*it->name=="\\prod");

	int total_number_of_indices=0; // refers to number of indices on gamma matrices
	exptree::sibling_iterator sib=it.begin();
	while(sib!=it.end()) {
		unsigned int i=0;
		if(exptree::number_of_indices(sib)==0) {
			++sib;
			continue;
			}
		if(properties::get_composite<GammaMatrix>(sib)) {
			total_number_of_indices+=tr.number_of_indices(sib);
			++sib;
			continue;
			}
		
		// In case of spinors, the name may be hidden inside a Dirac bar.
		exptree::sibling_iterator truetensor=sib;
		const DiracBar *db=properties::get_composite<DiracBar>(truetensor);
		if(db) 
			truetensor=tr.begin(truetensor);

		// Compare the current tensor with all other tensors encountered so far.
		for(; i<idts.size(); ++i) {
			exptree::sibling_iterator truetensor2=idts[i].tensors[0];
			const DiracBar *db2=properties::get_composite<DiracBar>(truetensor2);
			if(db2) 
				truetensor2=tr.begin(truetensor2);
			
			if(subtree_equal(truetensor2, truetensor)) {
				// If this is a spinor, check that it's connected to the one already stored
				// by a Gamma matrix, or that it is connected directly.
				if(idts[i].spino) {
					exptree::sibling_iterator tmpit=idts[i].tensors[0];
					const GammaMatrix *gmnxt=0;
					const Spinor      *spnxt=0;
					// skip objects without spinor line
					do { 
						++tmpit;
						gmnxt=properties::get_composite<GammaMatrix>(tmpit);
						spnxt=properties::get_composite<Spinor>(tmpit);
						} while(gmnxt==0 && spnxt==0);
					if(tmpit==sib) {
//						txtout << "using fermi exchange" << std::endl;
						idts[i].extra_sign++;
						break;
						}
					if(gmnxt) {
//						txtout << "gamma next " << std::endl;
						int numind=exptree::number_of_indices(tmpit);
						// skip objects without spinor line
						do {
							++tmpit;
							gmnxt=properties::get_composite<GammaMatrix>(tmpit);
							spnxt=properties::get_composite<Spinor>(tmpit);
							} while(gmnxt==0 && spnxt==0);
						if(tmpit==sib) { // yes, it's a proper Majorana spinor pair.
//							txtout << "using fermi exchange with gamma " << numind << std::endl;
							if( ((numind*(numind+1))/2)%2 == 0 )
								idts[i].extra_sign++;
							break;
							}
						}
					}
				else break;
				}
			}
		if(i==idts.size()) {
			identical_tensors_t ngr;
			ngr.comm=properties::get_composite<SelfCommutingBehaviour>(sib, true);
			ngr.spino=properties::get_composite<Spinor>(sib);
			ngr.tab=properties::get_composite<TableauBase>(sib);
			ngr.traceless=properties::get_composite<Traceless>(sib);
			ngr.gammatraceless=properties::get_composite<GammaTraceless>(sib);
			ngr.extra_sign=0;
			ngr.number_of_indices=exptree::number_of_indices(truetensor);
			ngr.tensors.push_back(sib);
			ngr.seq_numbers_of_first_indices.push_back(total_number_of_indices);
			total_number_of_indices+=ngr.number_of_indices;
			if(ngr.spino==false || ngr.spino->majorana==true)
				idts.push_back(ngr);
			}
		else {
			idts[i].tensors.push_back(sib);
			idts[i].seq_numbers_of_first_indices.push_back(total_number_of_indices);
			total_number_of_indices+=idts[i].number_of_indices;
			}
		++sib;
		}
	return total_number_of_indices;
	}


bool exchange::get_node_gs(exptree& tr, exptree::iterator it, std::vector<std::vector<int> >& gs)
	{
	std::vector<identical_tensors_t> idts;
	int total_number_of_indices=collect_identical_tensors(tr, it, idts);
	if(idts.size()==0) return true; // no indices, so nothing to permute

	// Make a strong generating set for the permutation of identical tensors.

	for(unsigned int i=0; i<idts.size(); ++i) {
		unsigned int num=idts[i].tensors.size();
		if(idts[i].comm)
			 if(idts[i].comm->sign()==0) continue;

		if(num>1) {
			std::vector<int> gsel(total_number_of_indices+2);

			for(unsigned int sobj=0; sobj<num-1; ++sobj) {
				for(int kk=0; kk<total_number_of_indices+2; ++kk)
					gsel[kk]=kk+1;
				
				// permutation of sobj & obj for all sobj and obj > sobj.
				for(unsigned int obj=sobj; obj<num-1; ++obj) {
					 for(unsigned int kk=0; kk<idts[i].number_of_indices; ++kk) 
						  std::swap(gsel[idts[i].seq_numbers_of_first_indices[obj]+kk], 
										gsel[idts[i].seq_numbers_of_first_indices[obj+1]+kk]);
					 if(idts[i].spino) {
						  assert(num==2); // FIXME: cannot yet do more than two fermions
						  if(idts[i].extra_sign%2==1) {
								std::swap(gsel[total_number_of_indices], gsel[total_number_of_indices+1]);
								}
						  }
					 if(idts[i].comm) {
						  if(idts[i].comm->sign()==-1) 
								std::swap(gsel[total_number_of_indices], gsel[total_number_of_indices+1]);
						  }
					 if(idts[i].spino && idts[i].number_of_indices==0) {
						  if(gsel[total_number_of_indices+1]==total_number_of_indices+1)
								return false;
						  }
					 else gs.push_back(gsel);
					 }
				 }
			 }
		 }

//  	for(unsigned int i=0; i<idts.size(); ++i) {
//  		int num=idts[i].tensors.size();
//  		if(num>1) {
//  			for(int t1=0; t1<num-1; ++t1) {
//  				for(int t2=t1+1; t2<num; ++t2) {
//  					std::vector<int> gsel(total_number_of_indices+2);
//  					for(int kk=0; kk<total_number_of_indices+2; ++kk)
//  						gsel[kk]=kk+1;
//  					
//  					if(idts[i].spino) {
//  						assert(num==2); // FIXME: cannot yet do more than two fermions
//  						if(idts[i].extra_sign%2==1) {
//  //							txtout << "extra sign" << std::endl;
//  							std::swap(gsel[total_number_of_indices], gsel[total_number_of_indices+1]);
//  							}
//  						}
//  					for(unsigned int kk=0; kk<idts[i].number_of_indices; ++kk) 
//  						std::swap(gsel[idts[i].seq_numbers_of_first_indices[t1]+kk], 
//  									 gsel[idts[i].seq_numbers_of_first_indices[t2]+kk]);
//  //						for(int kk=0; kk<total_number_of_indices+2; ++kk) {
//  //							txtout << gsel[kk] << " ";
//  //							}
//  //						txtout << std::endl;
//  //						txtout << "adding gs element" << std::endl;
//  
//  					if(idts[i].comm) {
//  						if(idts[i].comm->sign()==-1) {
//  //							txtout << "anticommuting" << std::endl;
//  							std::swap(gsel[total_number_of_indices], gsel[total_number_of_indices+1]);
//  							}
//  						else if(idts[i].comm->sign()==0)
//  							return false;
//  						}
//  					
//  					if(idts[i].spino && idts[i].number_of_indices==0) {
//  						if(gsel[total_number_of_indices+1]==total_number_of_indices+1)
//  							return false;
//  						}
//  					else gs.push_back(gsel);
//  					}
//  				}
//  			}
//  		}
	return true;
	}

bool operator<(const exchange::tensor_type_t& one, const exchange::tensor_type_t& two)
	{
	if(*one.name < *two.name) return true;
	if(one.name == two.name)
		if(one.number_of_indices < two.number_of_indices) return true;
	return false;
	}
