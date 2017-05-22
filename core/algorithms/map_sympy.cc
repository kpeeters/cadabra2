
#include "algorithms/map_sympy.hh"
#include "properties/Coordinate.hh"
#include "properties/Symbol.hh"
#include "SympyCdb.hh"

using namespace cadabra;

map_sympy::map_sympy(const Kernel& k, Ex& tr, const std::string& head)
	: Algorithm(k, tr), head_(head)
	{
	}

bool map_sympy::can_apply(iterator st)
	{
	index_map_t ind_free, ind_dummy;
	classify_indices(st, ind_free, ind_dummy);
	for(auto& ind: ind_free) {
		const Coordinate *cdn=kernel.properties.get_composite<Coordinate>(ind.second, true);
		const Symbol     *smb=kernel.properties.get_composite<Symbol>(ind.second, true);
		if(cdn==0 && smb==0)
			return false;
		}
	return (ind_dummy.size()==0);
	}

Algorithm::result_t map_sympy::apply(iterator& it)
	{
	std::vector<std::string> wrap;
	wrap.push_back(head_);
	sympy::apply(kernel, tr, it, wrap, "", "");
	it.skip_children();
	return result_t::l_applied;
	}
