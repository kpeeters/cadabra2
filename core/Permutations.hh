
#pragma once

#include <stdexcept>
#include <vector>

/// \ingroup core
///
/// Generic permutation group material. Largely follows the notation of xperm to avoid
/// confusion.
///
/// Example: 1->4, 4->3, 3->1, 5->6, 6->5
/// 
///    Perm(   3, 2, 4, 1, 6, 5 )
///    Images( 4, 2, 1, 3, 6, 5 )

class PermutationException : std::logic_error {
	public:
		PermutationException(const std::string& ex) : std::logic_error(ex) {};
};

class Perm {
	public:
		std::vector<int> perm;

		/// Find the permutation that takes [start1, end1> to [start2, end2>.
		template<class iterator>
		void find(iterator start1, iterator end1, iterator start2, iterator end2);

		/// Apply the permutation on the range. 
		template<class iterator>
		void apply(iterator start1, iterator end1);

};

class Images {
	public:
		std::vector<int> images;

		/// Find the permutation that takes [start1, end1> to [start2, end2>.
		template<class iterator>
		void find(iterator start1, iterator end1, iterator start2, iterator end2);
};




template<class iterator>
void Perm::find(iterator start1, iterator end1, iterator start2, iterator end2)
	{
	perm.clear();

	while(start2!=end2) {
		auto it=start1;
		int num=0;
		while(it!=end1) {
			if(*start2==*it) {
				perm.push_back(num);
				break;
				}
			++num;
			++it;
			}
		if(it==end1)
			throw PermutationException("Sets do not contain the same elements.");
		
		++start2;
		}
	}

template<class iterator>
void Perm::apply(iterator start, iterator end)
	{
	typedef typename std::remove_reference<decltype(*start)>::type value_type;

	std::vector<value_type> orig;
	auto it=start;
	while(it!=end) {
		orig.push_back(*it);
		++it;
		}

	// std::cerr << orig.size() << ", " << perm.size() << std::endl;
	assert(orig.size()==perm.size());

	for(unsigned int i=0; i<orig.size(); ++i) {
		*start=orig[perm[i]];
		++start;
		}
	}


template<class iterator>
void Images::find(iterator start1, iterator end1, iterator start2, iterator end2)
	{
	images.clear();

	while(start1!=end1) {
		auto it=start2;
		int num=0;
		while(it!=end2) {
			if(*start1==*it) {
				images.push_back(num);
				break;
				}
			++num;
			++it;
			}
		if(it==end2)
			throw PermutationException("Sets do not contain the same elements.");
		
		++start1;
		}
	}
