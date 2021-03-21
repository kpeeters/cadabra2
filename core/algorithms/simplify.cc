
#include "Config.hh"
#include "simplify.hh"
#include "Cleanup.hh"
#include "properties/Coordinate.hh"
#include "properties/Symbol.hh"
#include "SympyCdb.hh"
#ifdef MATHEMATICA_FOUND
#include "MMACdb.hh"
#endif

using namespace cadabra;

simplify::simplify(const Kernel& k, Ex& tr)
	: Algorithm(k, tr)
	{
	}

bool simplify::can_apply(iterator st)
	{
	// For \components nodes we need to map at the level of the individual
	// component values, not the top \components node.
	if(*st->name=="\\components") return false;
	if(*st->name=="\\equals") return false;
	if(*st->name=="\\comma") return false;
	if(*st->name=="\\wedge") return false;

	// If this is a sum, determine if there are any wedge products (we
	// do not pass enough info to sympy yet to enable it to simplify these).

	if(*st->name=="\\sum") {
		for(sibling_iterator sib=tr.begin(st), end=tr.end(st); sib!=end; ++sib)
			if(*sib->name=="\\wedge")
				return false;
		}

	// Check that any occuring indices are 'harmless', in the sense that
	// sympy knows how to handle them.
	
	left.clear();
	index_factors.clear();
	index_map_t ind_free, ind_dummy;
	classify_indices(st, ind_free, ind_dummy);

	bool still_ok=true;

	// Determine if any of the free indices are harmless (Coordinates or Symbols).
	for(auto& ind: ind_free) {
		const Coordinate *cdn=kernel.properties.get<Coordinate>(ind.second, true);
		const Symbol     *smb=kernel.properties.get<Symbol>(ind.second, true);
		if(cdn==0 && smb==0) {
			still_ok=false;
			break;
			}
		}

	if(still_ok && ind_dummy.size()==0) return true;

	// In a product, it is still possible that there is a sub-product which
	// contains no indices.
	if(*st->name=="\\prod") {
		// Find the factors in the product which have a proper index on them. Do this by
		// starting at the index, and if it is not coordinate or symbol, then go up until we
		// reach the first child level of the product.
		for(auto& ind: ind_free) {
			const Coordinate *cdn=kernel.properties.get<Coordinate>(ind.second, true);
			const Symbol     *smb=kernel.properties.get<Symbol>(ind.second, true);
			if(cdn==0 && smb==0) {
				auto fac=tr.parent(ind.second);
				while(tr.parent(fac)!=iterator(st))
					fac=tr.parent(fac);
				index_factors.insert(fac);
				}
			}
		for(auto& ind: ind_dummy) {
			const Coordinate *cdn=kernel.properties.get<Coordinate>(ind.second, true);
			const Symbol     *smb=kernel.properties.get<Symbol>(ind.second, true);
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

Algorithm::result_t simplify::apply(iterator& it)
	{
	std::vector<std::string> wrap;
	std::vector<std::string> args_;

	if(left.size()>0) {
		Ex prod("\\prod");
		for(auto& fac: left)
			prod.append_child(prod.begin(), fac);
		auto top=prod.begin();
		// std::cerr << "Feeding to sympy " << prod << std::endl;
		switch(kernel.scalar_backend) {
			case Kernel::scalar_backend_t::sympy:
				wrap.push_back("simplify");
					{
					ScopedProgressGroup group(pm, "sympy");
					sympy::apply(kernel, prod, top, wrap, args_, "");
					}
				break;
			case Kernel::scalar_backend_t::mathematica:
#ifdef MATHEMATICA_FOUND
				wrap.push_back("FullSimplify");
				//				args_.push_back("Trig -> False");
				{
					ScopedProgressGroup(pm, "mathematica");
					MMA::apply_mma(kernel, prod, top, wrap, args_, "");
				}
#endif
				break;
			}
		// Now remove the non-index carrying factors and replace with
		// the factors of 'prod' just simplified.
		tr.insert_subtree(*left.begin(), top);
		// std::cerr << "Before erasing " << Ex(it) << std::endl;
		for(auto& kl: left)
			tr.erase(kl);
		// std::cerr << "After erasing " << Ex(it) << std::endl;

		return result_t::l_applied;
		}
	else {
		switch(kernel.scalar_backend) {
			case Kernel::scalar_backend_t::sympy:
				wrap.push_back("simplify");
				if(pm) pm->group("sympy");
				sympy::apply(kernel, tr, it, wrap, args_, "");
				if(pm) pm->group();
				break;
			case Kernel::scalar_backend_t::mathematica:
#ifdef MATHEMATICA_FOUND
				wrap.push_back("FullSimplify");
				//				args_.push_back("Trig -> False");
				if(pm) pm->group("mathematica");
				MMA::apply_mma(kernel, tr, it, wrap, args_, "");
				if(pm) pm->group();
#endif
				break;
			}
		it.skip_children();
		return result_t::l_applied;
		}
	}
