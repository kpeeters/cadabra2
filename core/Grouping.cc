#include "Grouping.hh"
#include "Exceptions.hh"
#include "Compare.hh"
#include <map>

cadabra::equiv_map_t cadabra::group_by_equivalence(const Ex& ex, Ex::sibling_iterator first, Ex::sibling_iterator last)
	{
	equiv_fun_t comp = [](const Ex& ex, Ex::iterator it1, Ex::iterator it2) {
		return ex.equal_subtree(it1, it2);
		};

	return group_by_equivalence(ex, first, last, comp);
	}

cadabra::equiv_map_t cadabra::group_by_equivalence(const Ex& ex, Ex::sibling_iterator first, Ex::sibling_iterator last, equiv_fun_t& comp)
	{
	equiv_map_t equivs;
	
	for(auto it = first; it != last; ++it) {
		// std::cerr << it << std::endl;
		if( equivs.find(it) != equivs.end() )
			continue;
		
		auto it2=it;
		++it2;
		for(; it2!=last; ++it2) {
			if( equivs.find(it2) != equivs.end() )
				continue;
			
			if( comp(ex, it, it2) )
				equivs[it2] = std::make_pair( 1, it ); // FIXME: handle multiplier
			}
		}
		return equivs;
	}

cadabra::equiv_map_t cadabra::group_by_equivalence(const Ex& ex, Ex::iterator comma_top)
	{
	equiv_fun_t comp = [](const Ex& ex, Ex::iterator it1, Ex::iterator it2) {
		return ex.equal_subtree(it1, it2);
		};

	return group_by_equivalence(ex, comma_top, comp);
	}

cadabra::equiv_map_t cadabra::group_by_equivalence(const Ex& ex, Ex::iterator comma_top, equiv_fun_t& comp)
	{
	if(*comma_top->name!="\\comma")
		throw InternalError("cadabra::group_by_equivalence: top node not a list");

	return group_by_equivalence(ex, comma_top.begin(), comma_top.end(), comp);
	}
        
