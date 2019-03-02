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

/*
                               length vector
	normal combinations:     one element, value=total length.
	normal permutations:     n elements, each equal to 1.

*/

#pragma once

#include <vector>
#include <cassert>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>

namespace combin {

	typedef std::vector<unsigned int>  range_t;
	typedef std::vector<range_t>       range_vector_t;
	typedef std::vector<int>           weights_t;

	unsigned long factorial(unsigned int x);
	/// sum of elements
	long          vector_sum(const std::vector<int>&);
	/// product of elements
	unsigned long vector_prod(const std::vector<unsigned int>&);
	/// product of factorials of elements
	unsigned long vector_prod_fact(const std::vector<unsigned int>&);

	bool operator==(const std::vector<unsigned int>&, const std::vector<unsigned int>&);

	/// compute a hash value for a vector of unsigned ints
	long hash(const std::vector<unsigned int>&);

	template<class T>
	class combinations_base {
		public:
			combinations_base();
			combinations_base(const std::vector<T>&);
			virtual ~combinations_base();

			void         permute(long start=-1, long end=-1);
			virtual void clear();
			virtual void clear_results();
			unsigned int sum_of_sublengths() const;
			void         set_unit_sublengths();
			unsigned int multiplier(const std::vector<T>&) const;
			unsigned int total_permutations() const; // including the ones not stored

			enum weight_cond { weight_equals, weight_less, weight_greater };

			unsigned int               block_length;
			std::vector<unsigned int>  sublengths;
			range_vector_t             input_asym;
			std::vector<T>             original;
			bool                       multiple_pick;
			std::vector<weights_t>     weights;
			std::vector<int>           max_weights;
			std::vector<weight_cond>   weight_conditions;
			unsigned int               sub_problem_blocksize; // when non-zero, do permutations within

		protected:
			virtual void   vector_generated(const std::vector<unsigned int>&)=0;
			virtual bool   entry_accepted(unsigned int current) const;
			std::vector<unsigned int>  temparr;

			long                       start_, end_, vector_generated_called_;
			std::vector<int>           current_weight;
		private:
			bool is_allowed_by_weight_constraints(unsigned int i);
			bool final_weight_constraints_check() const;
			void update_weights(unsigned int i);
			void restore_weights(unsigned int i);
			void nextstep(unsigned int current, unsigned int fromalgehad, unsigned int groupindex,
			              std::vector<bool> algehad);

		};

	template<class T>
	class combinations : public combinations_base<T> {
		public:
			typedef typename std::vector<std::vector<T> >    permuted_sets_t;
			typedef typename permuted_sets_t::const_iterator const_iterator;

			combinations();
			combinations(const std::vector<T>&);
			virtual ~combinations();

			virtual void           clear();
			virtual void           clear_results();
			const std::vector<T>&  operator[](unsigned int) const;
			int                    ordersign(unsigned int) const;
			unsigned int           size() const;
			unsigned int           multiplier(unsigned int) const;

		protected:
			virtual void vector_generated(const std::vector<unsigned int>&);

		private:
			permuted_sets_t storage;
		};

	template<class T>
	class symmetriser;

	template<class T>
	class symm_helper : public combinations_base<T> {
		public:
			symm_helper(symmetriser<T>&);
			virtual void clear();

			int             current_multiplicity;
		protected:
			bool            first_one;
			symmetriser<T>& owner_;
			virtual void vector_generated(const std::vector<unsigned int>&);
		};

	template<class T>
	class symm_val_helper : public combinations_base<T> {
		public:
			symm_val_helper(symmetriser<T>&);
			virtual void clear();

			int             current_multiplicity;
		protected:
			bool            first_one;
			symmetriser<T>& owner_;
			virtual void vector_generated(const std::vector<unsigned int>&);
		};

	template<class T>
	class symmetriser {
		public:
			symmetriser();
			void apply_symmetry(long start=-1, long end=-1);

			std::vector<T>            original;
			unsigned int              block_length;
			std::vector<unsigned int> permute_blocks;   // offset in unit elements! (not in blocks)
			std::vector<T>            value_permute;
			int                       permutation_sign;

			std::vector<unsigned int> sublengths; // refers to position within permute_blocks
			range_vector_t            input_asym; // as in combinations_base
			range_vector_t            sublengths_scattered; // sublengths, in original, but not connected.

			/// Convert vectors of values to vectors of locations in the original
			/// (mainly useful to create input_asym for permutation by value).
			range_t                   values_to_locations(const std::vector<T>& values) const;

			const std::vector<T>&     operator[](unsigned int) const;
			int                       signature(unsigned int) const;
			void                      set_multiplicity(unsigned int pos, int val);
			unsigned int              size() const;
			void                      clear();
			/// Collect equal entries, and adjust the multiplier field accordingly.
			void                      collect();
			void                      remove_multiplicity_zero();

			friend class symm_helper<T>;
			friend class symm_val_helper<T>;
		private:
			symm_helper<T>               sh_;
			symm_val_helper<T>           svh_;
			unsigned int                 current_;
			std::vector<std::vector<T> > originals;
			std::vector<int>             multiplicity;
		};

	int determine_intersection_ranges(const range_vector_t& prod,
	                                  const range_vector_t& indv,
	                                  range_vector_t& target);

	template<class iterator1, class iterator2>
	int ordersign(iterator1 b1, iterator1 e1, iterator2 b2, iterator2 e2, int stepsize=1);

	template<class iterator1>
	int ordersign(iterator1 b1, iterator1 e1);

	template<class T>
	T fact(T x);

	template<class T>
	std::ostream& operator<<(std::ostream& str, const symmetriser<T>& sym);


	/*
	I assume PI consists of the integers 1 to N.
	It can be done with O(N) comparisons and transpositions of integers
	in the list.

	sign:= 1;
	for i from 1 to N do
	while PI[i] <> i do
	  	interchange PI[i] and PI[PI[i]];
	  	sign:= -sign
		od
	od
	*/

	template<class iterator1, class iterator2>
	int ordersign(iterator1 b1, iterator1 e1, iterator2 b2, iterator2 e2, int stepsize)
		{
		int sign=1;
		std::vector<bool> crossedoff(std::distance(b1,e1),false);
		while(b1!=e1) {
			int otherpos=0;
			iterator2 it=b2;
			while(it!=e2) {
				if( (*it)==(*b1) && crossedoff[otherpos]==false) {
					crossedoff[otherpos]=true;
					break;
					} else {
					if(!crossedoff[otherpos])
						sign=-sign;
					}
				it+=stepsize;
				++otherpos;
				}
			b1+=stepsize;
			}
		return sign;
		}

	//template<class iterator1, class iterator2, class comparator>
	//int ordersign(iterator1 b1, iterator1 e1, iterator2 b2, iterator2 e2, comparator cmp, int stepsize)
	//	{
	//	int sign=1;
	//	std::vector<bool> crossedoff(std::distance(b1,e1),false);
	//	while(b1!=e1) {
	//		int otherpos=0;
	//		iterator2 it=b2;
	//		while(it!=e2) {
	//			if(cmp((*it), (*b1)) && crossedoff[otherpos]==false) {
	//				crossedoff[otherpos]=true;
	//				break;
	//				}
	//			else {
	//				if(!crossedoff[otherpos])
	//					sign=-sign;
	//				}
	//			it+=stepsize;
	//			++otherpos;
	//			}
	//		b1+=stepsize;
	//		}
	//	return sign;
	//	}

	template<class iterator1>
	int ordersign(iterator1 b1, iterator1 e1)
		{
		std::vector<unsigned int> fil;
		for(int k=0; k<distance(b1,e1); ++k)
			fil.push_back(k);
		return ordersign(fil.begin(), fil.end(), b1, e1);
		}

	template<class T>
	T fact(T x)
		{
		T ret=1;
		assert(x>=0);
		while(x!=0) {
			ret*=x--;
			}
		return ret;
		}


	// Implementations

	template<class T>
	combinations_base<T>::combinations_base()
		: block_length(1), multiple_pick(false), sub_problem_blocksize(0)
		{
		}

	template<class T>
	combinations_base<T>::combinations_base(const std::vector<T>& oa)
		: block_length(1), original(oa), multiple_pick(false), sub_problem_blocksize(0)
		{
		}

	template<class T>
	combinations<T>::combinations()
		: combinations_base<T>()
		{
		}

	template<class T>
	combinations<T>::combinations(const std::vector<T>& oa)
		: combinations_base<T>(oa)
		{
		}

	template<class T>
	combinations_base<T>::~combinations_base()
		{
		}

	template<class T>
	combinations<T>::~combinations()
		{
		}

	template<class T>
	void combinations<T>::vector_generated(const std::vector<unsigned int>& toadd)
		{
		++this->vector_generated_called_;
		if((this->start_==-1 || this->vector_generated_called_ >= this->start_) &&
		      (this->end_==-1   || this->vector_generated_called_ < this->end_)) {
			std::vector<T> newone(toadd.size()*this->block_length);
			for(unsigned int i=0; i<toadd.size(); ++i)
				for(unsigned int bl=0; bl<this->block_length; ++bl)
					newone[i*this->block_length+bl]=this->original[toadd[i]*this->block_length+bl];
			storage.push_back(newone);
			}
		}

	template<class T>
	bool combinations_base<T>::entry_accepted(unsigned int) const
		{
		return true;
		}

	template<class T>
	void combinations_base<T>::permute(long start, long end)
		{
		start_=start;
		end_=end;
		vector_generated_called_=-1;

		// Initialise weight handling.
		current_weight.clear();
		current_weight.resize(weights.size(), 0);
		for(unsigned int i=0; i<weights.size(); ++i)
			assert(weights[i].size() == original.size()/block_length);
		if(weights.size()>0) {
			if(weight_conditions.size()==0)
				weight_conditions.resize(weights.size(), weight_equals);
			else assert(weight_conditions.size()==weights.size());
			} else assert(weight_conditions.size()==0);

		// Sublength handling.
		assert(sublengths.size()!=0);
		unsigned int len=sum_of_sublengths();

		// Consistency checks.
		assert(original.size()%block_length==0);
		if(!multiple_pick)
			assert(len*block_length<=original.size());

		for(unsigned int i=0; i<this->input_asym.size(); ++i)
			std::sort(this->input_asym[i].begin(), this->input_asym[i].end());

		temparr=std::vector<unsigned int>(len/* *block_length*/);
		std::vector<bool> algehad(original.size()/block_length,false);
		nextstep(0,0,0,algehad);
		}

	template<class T>
	void combinations_base<T>::clear()
		{
		block_length=1;
		sublengths.clear();
		this->input_asym.clear();
		original.clear();
		weights.clear();
		max_weights.clear();
		weight_conditions.clear();
		sub_problem_blocksize=0;
		temparr.clear();
		current_weight.clear();
		}

	template<class T>
	void combinations_base<T>::clear_results()
		{
		temparr.clear();
		}

	template<class T>
	void combinations<T>::clear()
		{
		storage.clear();
		combinations_base<T>::clear();
		}

	template<class T>
	void combinations<T>::clear_results()
		{
		storage.clear();
		combinations_base<T>::clear_results();
		}

	template<class T>
	const std::vector<T>& combinations<T>::operator[](unsigned int i) const
		{
		assert(i<storage.size());
		return storage[i];
		}

	template<class T>
	unsigned int combinations<T>::size() const
		{
		return storage.size();
		}

	template<class T>
	unsigned int combinations_base<T>::sum_of_sublengths() const
		{
		unsigned int ret=0;
		for(unsigned int i=0; i<sublengths.size(); ++i)
			ret+=sublengths[i];
		return ret;
		}

	template<class T>
	unsigned int combinations_base<T>::total_permutations() const
		{
		return vector_generated_called_+1;
		}

	template<class T>
	void combinations_base<T>::set_unit_sublengths()
		{
		sublengths.clear();
		for(unsigned int i=0; i<original.size()/block_length; ++i)
			sublengths.push_back(1);
		}

	template<class T>
	int combinations<T>::ordersign(unsigned int num) const
		{
		assert(num<storage.size());
		return combin::ordersign(storage[0].begin(), storage[0].end(),
		                         storage[num].begin(), storage[num].end(), this->block_length);
		}

	template<class T>
	unsigned int combinations<T>::multiplier(unsigned int num) const
		{
		return combinations_base<T>::multiplier(this->storage[num]);
		}

	template<class T>
	unsigned int combinations_base<T>::multiplier(const std::vector<T>& stor) const
		{
		unsigned long numerator=1;
		for(unsigned int i=0; i<this->input_asym.size(); ++i)
			numerator*=fact(this->input_asym[i].size());

		unsigned long denominator=1;
		for(unsigned int i=0; i<this->input_asym.size(); ++i) {
			// for each input asym, and for each output asym, count
			// the number of overlap elements.
			unsigned int current=0;
			for(unsigned int k=0; k<this->sublengths.size(); ++k) {
				if(this->sublengths[k]>1) {
					unsigned int overlap=0;
					for(unsigned int slc=0; slc<this->sublengths[k]; ++slc) {
						for(unsigned int j=0; j<this->input_asym[i].size(); ++j) {
							unsigned int index=0;
							while(!(stor[current]==this->original[index]))
								++index;
							if(index==this->input_asym[i][j])
								++overlap;
							}
						++current;
						}
					if(overlap>0)
						denominator*=fact(overlap);
					// FIXME: for each overlap thus found, divide out by a factor
					// due to the fact that output asym ranges can overlap.
					// well, that's not right either.
					} else ++current;
				}
			}

		return numerator/denominator;
		}

	template<class T>
	bool combinations_base<T>::is_allowed_by_weight_constraints(unsigned int i)
		{
		if(weights.size()==0) return true;
		for(unsigned int cn=0; cn<current_weight.size(); ++cn) {
			if(weight_conditions[cn]==weight_less)
				if(current_weight[cn]+weights[cn][i] >= max_weights[cn])
					return false;
			}
		return true;
		}

	template<class T>
	bool combinations_base<T>::final_weight_constraints_check() const
		{
		for(unsigned int cn=0; cn<current_weight.size(); ++cn) {
			switch(weight_conditions[cn]) {
			case weight_equals:
				if(current_weight[cn]!=max_weights[cn])
					return false;
				break;
			case weight_less:
				break;
			case weight_greater:
				if(current_weight[cn]<=max_weights[cn])
					return false;
				break;
				}
			}
		return true;
		}

	template<class T>
	void combinations_base<T>::update_weights(unsigned int i)
		{
		if(weights.size()==0) return;
		for(unsigned int cn=0; cn<current_weight.size(); ++cn)
			current_weight[cn]+=weights[cn][i];
		}

	template<class T>
	void combinations_base<T>::restore_weights(unsigned int i)
		{
		if(weights.size()==0) return;
		for(unsigned int cn=0; cn<current_weight.size(); ++cn)
			current_weight[cn]-=weights[cn][i];
		}

	template<class T>
	void combinations_base<T>::nextstep(unsigned int current, unsigned int lowest_in_group, unsigned int groupindex,
	                                    std::vector<bool> algehad)
		{
		unsigned int grouplen=0;
		for(unsigned int i=0; i<=groupindex; ++i)
			grouplen+=sublengths[i];
		if(current==grouplen) { // group is filled
			++groupindex;
			if(groupindex==sublengths.size()) {
				if(final_weight_constraints_check())
					vector_generated(temparr);
				return;
				}
			lowest_in_group=0;
			}

		unsigned int starti=0, endi=original.size()/block_length;
		if(sub_problem_blocksize>0) {
			starti=current-current%sub_problem_blocksize;
			endi=starti+sub_problem_blocksize;
			}
		for(unsigned int i=starti; i<endi; i++) {
			if(!algehad[i] || multiple_pick) {
				bool discard=false;
				if(is_allowed_by_weight_constraints(i)) {
					// handle input_asym
					for(unsigned k=0; k<this->input_asym.size(); ++k) {
						for(unsigned int kk=0; kk<this->input_asym[k].size(); ++kk) {
							if(i==this->input_asym[k][kk]) {
								unsigned int k2=kk;
								while(k2!=0) {
									--k2;
									if(!algehad[this->input_asym[k][k2]]) {
										//									std::cout << "discarding " << std::endl;
										discard=true;
										break;
										}
									}
								}
							}
						if(discard) break;
						}
					} else discard=true;
				if(!discard)
					if(i+1>lowest_in_group) {
						algehad[i]=true;
						update_weights(i);
						temparr[current]=i;
						//					for(unsigned bl=0; bl<block_length; ++bl)
						//						temparr[current*block_length+bl]=original[i*block_length+bl];
						if(entry_accepted(current)) {
							nextstep(current+1, i, groupindex, algehad);
							}
						algehad[i]=false;
						restore_weights(i);
						}
				}
			}
		}

	template<class T>
	symmetriser<T>::symmetriser()
		: block_length(1), permutation_sign(1), sh_(*this), svh_(*this)
		{
		}

	template<class T>
	void symmetriser<T>::clear()
		{
		original.clear();
		block_length=1;
		permute_blocks.clear();
		value_permute.clear();
		permutation_sign=1;
		sublengths.clear();
		input_asym.clear();
		sublengths_scattered.clear();
		originals.clear();
		multiplicity.clear();
		}

	template<class T>
	void symmetriser<T>::collect()
		{
		std::cout << "collecting" << std::endl;
		// Fill the hash map: entries which are equal have to sit in the same
		// bin, but there may be other entries in that bin which still have to
		// be separated.
		std::multimap<long, unsigned int> hashmap;
		for(unsigned int i=0; i<originals.size(); ++i)
			hashmap.insert(std::pair<long, unsigned int>(hash(originals[i]), i));

		// Collect equal vectors.
		std::multimap<long, unsigned int>::iterator it=hashmap.begin(), thisbin1, thisbin2, tmpit;
		while(it!=hashmap.end()) {
			long current_hash=it->first;
			thisbin1=it;
			while(thisbin1!=hashmap.end() && thisbin1->first==current_hash) {
				thisbin2=thisbin1;
				++thisbin2;
				while(thisbin2!=hashmap.end() && thisbin2->first==current_hash) {
					if(originals[(*thisbin1).second]==originals[(*thisbin2).second]) {
						multiplicity[(*thisbin1).second]+=multiplicity[(*thisbin2).second];
						multiplicity[(*thisbin2).second]=0;
						tmpit=thisbin2;
						++tmpit;
						hashmap.erase(thisbin2);
						thisbin2=tmpit;
						} else ++thisbin2;
					}
				++thisbin1;
				}
			it=thisbin1;
			}

		remove_multiplicity_zero();
		}

	template<class T>
	void symmetriser<T>::remove_multiplicity_zero()
		{
		std::vector<std::vector<T> > new_originals;
		std::vector<int>             new_multiplicity;
		for(unsigned int k=0; k<originals.size(); ++k) {
			if(multiplicity[k]!=0) {
				new_originals.push_back(originals[k]);
				new_multiplicity.push_back(multiplicity[k]);
				}
			}
		originals=new_originals;
		multiplicity=new_multiplicity;
		}


	template<class T>
	void symmetriser<T>::apply_symmetry(long start, long end)
		{
		unsigned int current_length=originals.size();
		if(current_length==0) {
			originals.push_back(original);
			multiplicity.push_back(1);
			current_length=1;
			}

		// Some options are mutually exclusive.
		assert(permute_blocks.size()>0 || value_permute.size()>0);
		assert(sublengths.size()==0 || sublengths_scattered.size()==0);

		if(permute_blocks.size()==0) { // permute by value
			assert(value_permute.size()!=0);

			if(input_asym.size()==0 && sublengths_scattered.size()==0) {
				// When permuting by value, we can do the permutation once,
				// and then figure out (see vector_generated of symm_val_helper),
				// for each permutation which is already stored in the symmetriser,
				// how the objects are moved.

				current_=current_length;
				svh_.clear();
				svh_.original=value_permute;
				svh_.input_asym.clear();
				svh_.sublengths=sublengths;
				svh_.current_multiplicity=combin::vector_prod_fact(sublengths);
				if(svh_.sublengths.size()==0)
					svh_.set_unit_sublengths();

				svh_.permute(start, end);
				// Since we do not divide by the number of permutations, we need
				// to adjust the multiplicity of all the originals.
				//		for(unsigned int i=0; i<current_; ++i)
				//				multiplicity[i] *= svh_.current_multiplicity;
				} else {
				// However, when there is input_asym or sublength_scattered
				// are present, we cannot just do the permutation on the
				// values and then put them into all existing sets, since the
				// overlap of input_asym with the objects to be permuted will
				// be different for every set.  Therefore, we have to apply
				// the permutation algorithm separately to each and every set
				// which is already stored in the symmetriser.  We convert
				// the problem to a permute-by-location problem.

				for(unsigned int i=0; i<current_length; ++i) {
					current_=i;
					sh_.clear();
					assert(sublengths.size()==0); // not yet implemented
					std::vector<unsigned int> my_permute_blocks;

					// Determine the location of the values.
					for(unsigned int k=0; k<value_permute.size(); ++k) {
						for(unsigned int m=0; m<originals[i].size(); ++m) {
							if(originals[i][m]==value_permute[k]) {
								my_permute_blocks.push_back(m); // FIXME: non-unit block length?
								break;
								}
							}
						}

					//				std::cout << "handling sublengths" << std::endl;
					if(sublengths_scattered.size()>0) {
						// Re-order my_permute_blocks in such a way that the objects which sit
						// in one sublength_scattered range are consecutive. This does not make
						// any difference for the sign.
						sh_.sublengths.clear();
						std::vector<unsigned int> reordered_permute_blocks;
						for(unsigned int m=0; m<sublengths_scattered.size(); ++m) {
							int overlap=0;
							for(unsigned int mm=0; mm<sublengths_scattered[m].size(); ++mm) {
								//							std::cout << "trying to find " << sublengths_scattered[m][mm] << " " << std::flush;
								std::vector<unsigned int>::iterator it=my_permute_blocks.begin();
								while(it!=my_permute_blocks.end()) {
									if((*it)==sublengths_scattered[m][mm]) {
										//									std::cout << " found " << std::endl;
										reordered_permute_blocks.push_back(*it);
										my_permute_blocks.erase(it);
										++overlap;
										break;
										}
									++it;
									}
								//							std::cout << std::endl;
								}
							if(overlap>0)
								sh_.sublengths.push_back(overlap);
							}
						std::vector<unsigned int>::iterator it=my_permute_blocks.begin();
						while(it!=my_permute_blocks.end()) {
							reordered_permute_blocks.push_back(*it);
							//						std::cout << "adding one" << std::endl;
							sh_.sublengths.push_back(1);
							++it;
							}
						my_permute_blocks=reordered_permute_blocks;
						//					std::cout << "handled sublengths" << std::endl;
						}

					// Put to-be-permuted data in originals.
					for(unsigned int k=0; k<my_permute_blocks.size(); ++k) {
						for(unsigned int kk=0; kk<block_length; ++kk) {
							sh_.original.push_back(originals[i][my_permute_blocks[k]+kk]);
							}
						}

					combin::range_vector_t subprob_input_asym;
					sh_.current_multiplicity=1;
					if(input_asym.size()>0) {
						// Make a proper input_asym which refers to object locations
						// in the permute blocks array, rather than in the original
						// array.
						for(unsigned int k=0; k<input_asym.size(); ++k) {
							range_t newrange;
							for(unsigned int m=0; m<input_asym[k].size(); ++m) {
								// search in my_permute_blocks
								for(unsigned int kk=0; kk<my_permute_blocks.size(); ++kk)
									if(my_permute_blocks[kk]==input_asym[k][m]) {
										newrange.push_back(kk);
										break;
										}
								}
							if(newrange.size()>1) {
								subprob_input_asym.push_back(newrange);
								sh_.current_multiplicity*=fact(newrange.size());
								}
							}
						}
					if(sh_.sublengths.size()==0)
						sh_.set_unit_sublengths();
					sh_.current_multiplicity*=combin::vector_prod_fact(sh_.sublengths);

					// debugging
					//				std::cout << "my_permute_blocks: ";
					//				for(unsigned int ii=0; ii<my_permute_blocks.size(); ++ii)
					//					std::cout << my_permute_blocks[ii] << " ";
					//				std::cout << std::endl;
					//				std::cout << "sublengths: ";
					//				for(unsigned int ii=0; ii<sh_.sublengths.size(); ++ii)
					//					std::cout << sh_.sublengths[ii] << " ";
					//				std::cout << std::endl;

					// Debugging output:
					//				std::cout << sh_.current_multiplicity << " asym: ";
					//				if(subprob_input_asym.size()>0) {
					//					for(unsigned int k=0; k<subprob_input_asym[0].size(); ++k)
					//						std::cout << subprob_input_asym[0][k] << " ";
					//					std::cout << std::endl;
					//					std::cout << subprob_input_asym.size() << std::endl;
					//					}
					//				else std::cout << "no asym" << std::endl;

					permute_blocks=my_permute_blocks;
					sh_.block_length=block_length;
					sh_.input_asym=subprob_input_asym;
					sh_.permute(start, end);

					// Since we do not divide by the number of permutations, we need
					// to adjust the multiplicity of the original.
					multiplicity[i]*=sh_.current_multiplicity;
					permute_blocks.clear(); // restore just in case
					//				for(unsigned int m=0; m<originals.size(); ++m) {
					//					for(unsigned int mm=0; mm<originals[m].size(); ++mm)
					//						std::cout << originals[m][mm] << " ";
					//					std::cout << std::endl;
					//					}
					//				break;
					}
				}
			} else {                       // permute by location
			assert(value_permute.size()==0);
			assert(permute_blocks.size()>0);
			// When permuting by location, we have to apply the permutation
			// algorithm separately to each and every permutation which is
			// already stored in the symmetriser.
			for(unsigned int i=0; i<current_length; ++i) {
				current_=i;
				sh_.clear();
				for(unsigned int k=0; k<permute_blocks.size(); ++k) {
					for(unsigned int kk=0; kk<block_length; ++kk) {
						sh_.original.push_back(originals[i][permute_blocks[k]+kk]);
						}
					//				sh_.sublengths.push_back(1);
					}
				assert(sublengths.size()==0); // not yet implemented
				// sh_.sublengths=sublengths;
				if(sh_.sublengths.size()==0)
					sh_.set_unit_sublengths();
				sh_.block_length=block_length;
				sh_.input_asym=input_asym;
				sh_.permute(start, end);
				}
			}

		if(start!=-1) { // if start is not the first, have to erase the first
			originals.erase(originals.begin());
			multiplicity.erase(multiplicity.begin());
			}
		}

	template<class T>
	const std::vector<T>& symmetriser<T>::operator[](unsigned int i) const
		{
		assert(i<originals.size());
		return originals[i];
		}

	template<class T>
	unsigned int symmetriser<T>::size() const
		{
		return originals.size();
		}

	template<class T>
	range_t symmetriser<T>::values_to_locations(const std::vector<T>& values) const
		{
		range_t ret;
		for(unsigned int i=0; i<values.size(); ++i) {
			//		std::cout << "finding " << values[i] << std::endl;
			for(unsigned int j=0; j<value_permute.size(); ++j) {
				//			std::cout << value_permute[j] << " ";
				if(value_permute[i]==value_permute[j]) {
					//				std::cout << "found" << std::endl;
					ret.push_back(j);
					break;
					}
				//			std::cout << std::endl;
				}
			}
		return ret;
		}

	template<class T>
	symm_val_helper<T>::symm_val_helper(symmetriser<T>& tt)
		: current_multiplicity(1), first_one(true), owner_(tt)
		{
		}

	template<class T>
	void symm_val_helper<T>::clear()
		{
		first_one=true;
		combinations_base<T>::clear();
		}

	template<class T>
	void symm_val_helper<T>::vector_generated(const std::vector<unsigned int>& vec)
		{
		++this->vector_generated_called_;
		if(first_one) {
			first_one=false;
			} else {
			if((this->start_==-1 || this->vector_generated_called_ >= this->start_) &&
			      (this->end_==-1   || this->vector_generated_called_ < this->end_)) {

				// Since we permuted by value, we can do this permutation in one
				// shot on all previously generated sets.
				for(unsigned int i=0; i<owner_.current_; ++i) {
					//				owner_.multiplicity[i] *= current_multiplicity;
					owner_.originals.push_back(owner_.originals[i]);

					// Take care of the multiplicity & sign.
					int multiplicity=owner_.multiplicity[i] * current_multiplicity;
					if(owner_.permutation_sign==-1)
						multiplicity*=ordersign(vec.begin(), vec.end());
					owner_.multiplicity.push_back(multiplicity); //sign==1?true:false);

					// We now have to find the permuted objects in the larger
					// "original" set, and re-order these appropriately.
					unsigned int loc=owner_.originals.size()-1;
					for(unsigned int j=0; j<vec.size(); ++j) {
						for(unsigned int k=0; k<owner_.originals[i].size(); ++k) {
							if(owner_.originals[i][k]==this->original[j]) {
								owner_.originals[loc][k]=this->original[vec[j]];
								break;
								}
							}
						}
					}
				}
			}
		}

	template<class T>
	symm_helper<T>::symm_helper(symmetriser<T>& tt)
		: current_multiplicity(1), first_one(true), owner_(tt)
		{
		}

	template<class T>
	void symm_helper<T>::clear()
		{
		first_one=true;
		combinations_base<T>::clear();
		}

	template<class T>
	int symmetriser<T>::signature(unsigned int i) const
		{
		assert(i<multiplicity.size());
		return multiplicity[i]; //?1:-1;
		}

	template<class T>
	void symmetriser<T>::set_multiplicity(unsigned int i, int val)
		{
		assert(i<multiplicity.size());
		multiplicity[i]=val;
		}

	template<class T>
	void symm_helper<T>::vector_generated(const std::vector<unsigned int>& vec)
		{
		++this->vector_generated_called_;
		if(first_one) {
			first_one=false;
			} else {
			if((this->start_==-1 || this->vector_generated_called_ >= this->start_) &&
			      (this->end_==-1   || this->vector_generated_called_ < this->end_)) {

				//			std::cout << "produced ";
				//			for(unsigned int m=0; m<vec.size(); ++m)
				//			std::cout << vec[m] << " ";
				//			std::cout << std::endl;

				//			owner_.multiplicity[owner_.current_] *= current_multiplicity;
				owner_.originals.push_back(owner_.originals[owner_.current_]);
				unsigned int siz=owner_.originals.size()-1;

				// Take care of the permutation sign.
				int multiplicity=owner_.multiplicity[owner_.current_] * current_multiplicity;
				if(owner_.permutation_sign==-1)
					multiplicity*=ordersign(vec.begin(), vec.end());
				owner_.multiplicity.push_back(multiplicity);

				for(unsigned int k=0; k<owner_.permute_blocks.size(); ++k) {
					for(unsigned int kk=0; kk<owner_.block_length; ++kk) {
						assert(owner_.permute_blocks[k]+kk<owner_.originals[0].size());
						owner_.originals[siz][owner_.permute_blocks[k]+kk]=
						   owner_.originals[owner_.current_][owner_.permute_blocks[vec[k]]+kk];
						}
					}
				}
			}
		}

	template<class T>
	std::ostream& operator<<(std::ostream& str, const symmetriser<T>& sym)
		{
		for(unsigned int i=0; i<sym.size(); ++i) {
			for(unsigned int j=0; j<sym[i].size(); ++j) {
				str << sym[i][j] << " ";
				}
			str << " ";
			str.setf(std::ios::right, std::ios::adjustfield);
			str << std::setw(2) << sym.signature(i) << std::endl;
			}
		return str;
		}

	}



