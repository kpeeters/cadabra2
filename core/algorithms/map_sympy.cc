
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
	left.clear();
	index_factors.clear();
	index_map_t ind_free, ind_dummy;
	classify_indices(st, ind_free, ind_dummy);

	bool still_ok=true;

	// Determine if any of the free indices are harmless (Coordinates or Symbols).
	for(auto& ind: ind_free) {
		const Coordinate *cdn=kernel.properties.get_composite<Coordinate>(ind.second, true);
		const Symbol     *smb=kernel.properties.get_composite<Symbol>(ind.second, true);
		if(cdn==0 && smb==0) {
			still_ok=false;
			break;
			}
		}

	if(still_ok && ind_dummy.size()==0) return true;
	
   // In a product, it is still possible that there is a sub-product which
	// contains no indices.
	if(*st->name=="\\prod") {
		for(auto& ind: ind_free) {
			const Coordinate *cdn=kernel.properties.get_composite<Coordinate>(ind.second, true);
			const Symbol     *smb=kernel.properties.get_composite<Symbol>(ind.second, true);
			if(cdn==0 && smb==0) {
				auto fac=tr.parent(ind.second);
				while(tr.parent(fac)!=iterator(st))
					fac=tr.parent(fac);
				index_factors.insert(fac);
				}
			}
		for(auto& ind: ind_dummy) {
			const Coordinate *cdn=kernel.properties.get_composite<Coordinate>(ind.second, true);
			const Symbol     *smb=kernel.properties.get_composite<Symbol>(ind.second, true);
			if(cdn==0 && smb==0) {
				auto fac=tr.parent(ind.second);
				while(tr.parent(fac)!=iterator(st))
					fac=tr.parent(fac);
				index_factors.insert(fac);
				}
			}
		sibling_iterator sib=tr.begin(st);
		while(sib!=tr.end(st)) {
			if(index_factors.find(iterator(sib))==index_factors.end())
				left.push_back(sib);
			++sib;
			}
		return left.size()>0;
		}

	return false;
	}

Algorithm::result_t map_sympy::apply(iterator& it)
	{
	std::vector<std::string> wrap;
	wrap.push_back(head_);

	if(left.size()>0) {
		std::cerr << "Sub-product with " << left.size() << " non-index carrying factors" << std::endl;
		Ex prod("\\prod");
		for(auto& fac: left)
			prod.append_child(fac);
		auto top=prod.begin();
		sympy::apply(kernel, prod, top, wrap, "", "");
		for(auto& kl: index_factors)
			tr.erase(kl);
		
		return result_t::l_no_action;
		}
	else {
		sympy::apply(kernel, tr, it, wrap, "", "");
		it.skip_children();
		return result_t::l_applied;
		}
	}
