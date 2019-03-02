
#include "Permutations.hh"
#include <iostream>

int main()
	{
	std::vector<int> one{ 3,6,8,9,2 };
	std::vector<int> two{ 8,6,9,2,3 };

	Perm perm;
	perm.find_permutation(one.begin(), one.end(), two.begin(), two.end());

	for(auto& i: perm.perm)
		std::cout << i << ", ";

	std::cout << std::endl;
	}
