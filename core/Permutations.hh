
#pragma once

/// \ingroup core
///
/// Generic permutation group material. Largely follows the notation of xperm to avoid
/// confusion.
///
/// Example: 1->4, 4->3, 3->1, 5->6, 6->5
/// 
///    Perm(   3, 2, 4, 1, 6, 5 )
///    Images( 4, 2, 1, 3, 6, 5 )

class Perm {
	public:
		std::vector<int> perm;

		/// Find the permutation that takes the first range to the second.
		template<class iterator>
		void find_permutation(iterator start1, iterator end1, iterator start2);
};

class Images {
	public:
};
