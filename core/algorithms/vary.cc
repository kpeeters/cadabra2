
#include "Cleanup.hh"
#include "Exceptions.hh"
#include "algorithms/vary.hh"
#include "algorithms/substitute.hh"
#include "algorithms/product_rule.hh"
#include "properties/Derivative.hh"
#include "properties/Accent.hh"

using namespace cadabra;

// #define DEBUG 1

vary::vary(const Kernel& k, Ex& tr, Ex& args_)
	: Algorithm(k, tr), args(args_)
	{
	}

bool vary::can_apply(iterator it)
	{
	if(it->is_zero()) return false;
	
	if(*it->name=="\\prod") return true;
	if(*it->name=="\\commutator") return true;
	if(*it->name=="\\anticommutator") return true;
	if(*it->name=="\\sum") return true;
	if(*it->name=="\\pow") return true;
	if(*it->name=="\\int") return true;
	if(*it->name=="\\equals") return false;
	if(is_single_term(it)) return true;
	if(is_nonprod_factor_in_prod(it)) return true;
	const Derivative *der = kernel.properties.get<Derivative>(it);
	if(der) return true;
	const Accent *acc = kernel.properties.get<Accent>(it);
	if(acc) return true;

	if(!tr.is_head(it)) {
		der = kernel.properties.get<Derivative>(tr.parent(it));
		if(der) return true;
		acc = kernel.properties.get<Accent>(tr.parent(it));
		if(acc) return true;
		}
	return false;
	}

/*

D(A) C + D(A);
@vary(%)(


*/

Algorithm::result_t vary::apply(iterator& it)
	{
	result_t res=result_t::l_no_action;

	if(*it->name=="\\prod" || *it->name=="\\commutator" || *it->name=="\\anticommutator") {
		Ex result;
		result.set_head(str_node("\\sum"));
		iterator newsum=result.begin();

		// Iterate over all factors, attempting a substitute. If this
		// succeeds, copy the term to the "result" tree. Then restore the
		// original. We have to do the substitute on the original tree so
		// that index relabelling takes into account the rest of the tree.

		Ex prodcopy(it); // keep a copy to restore after each substitute

		vary varyfac(kernel, tr, args);
		int pos=0;
		for(;;) {
			sibling_iterator fcit=tr.begin(it);
			fcit+=pos;
			if(fcit==tr.end(it)) break;

			iterator fcit2(fcit);
			if(varyfac.can_apply(fcit2)) {
				result_t res = varyfac.apply(fcit2);
				if(fcit2->is_zero()==false && res==result_t::l_applied) {
					auto toclean = result.append_child(newsum, it);
					// the varied factor itself cannot get rid of nested
					// products, that's for us to do at the top level prod.
					cleanup_dispatch(kernel, tr, toclean);
					// std::cerr << toclean << std::endl;
					}

				// restore original
				it=tr.replace(it, prodcopy.begin());
				}
			++pos;
			}
		if(tr.number_of_children(newsum)>0) {
			it=tr.move_ontop(it, newsum);
			}
		else {   // varying any of the factors produces nothing, variation is zero
			zero(it->multiplier);
			}
//		std::cerr << it << std::endl;
		cleanup_dispatch(kernel, tr, it);
//		std::cerr << it << std::endl;		
		res=result_t::l_applied;
		return res;
		}

	const Derivative *der = kernel.properties.get<Derivative>(it);
	const Accent     *acc = kernel.properties.get<Accent>(it);
	if(der || acc) {
		vary vry(kernel, tr, args);

		sibling_iterator sib=tr.begin(it);
		bool has_applied=false;
		while(sib!=tr.end(it)) {
			iterator app=sib;
			++sib;
			if(app->is_index()) continue;
			if(vry.can_apply(app)) {
				if(vry.apply(app)==result_t::l_applied) {
					has_applied=true;
					res=result_t::l_applied;
					}
				}
			}

		// If no variation took place, set to zero if we are termlike.
		if(!has_applied && is_termlike(it)) {
			zero(it->multiplier);
			return result_t::l_applied;
			}

		return res;
		}

	if(*it->name=="\\sum") { // call vary on every term
		vary vry(kernel, tr, args);

		sibling_iterator sib=tr.begin(it);
		while(sib!=tr.end(it)) {
			iterator app=sib;
			++sib;
			if(vry.can_apply(app)) {
				res = vry.apply(app);
				}
			else {
				// remove this term
				res=result_t::l_applied;
				node_zero(app);
				}
			}

		cleanup_dispatch(kernel, tr, it);
		return res;
		}

	if(*it->name=="\\int") { // call vary on first argument
		vary vry(kernel, tr, args);

		iterator sib=tr.begin(it);
		if(vry.can_apply(sib))
			res=vry.apply(sib);
		return res;
		}

	if(*it->name=="\\pow") {
		Ex backup(it);
		// Wrap the power in a \cdbDerivative and then call @prodrule.
		it=tr.wrap(it, str_node("\\cdbDerivative"));
		product_rule pr(kernel, tr);
		pr.can_apply(it);
		pr.apply(it);
		// Find the '\cdbDerivative node again'.
		sibling_iterator sib=tr.begin(it);
		res=result_t::l_no_action;
		while(sib!=tr.end(it)) {
			if(*sib->name=="\\cdbDerivative") {
				tr.flatten(sib);
				sib=tr.erase(sib);
				vary vry(kernel, tr, args);
				iterator app=sib;
				if(vry.can_apply(app)) {
					res=vry.apply(app);
					}
				break;
				}
			++sib;
			}
		if(res!=result_t::l_applied) {
			// restore original
//			it=tr.replace(it, backup.begin());
			zero(it->multiplier);
			}
		return res;
		}

	if(tr.is_head(it)==false) {
		der = kernel.properties.get<Derivative>(tr.parent(it));
		acc = kernel.properties.get<Accent>(tr.parent(it));

		if(der || acc || is_single_term(it)) { // easy: just vary this term by substitution
			// std::cerr << "single term " << *it->name << std::endl;
			substitute subs(kernel, tr, args);
			if(subs.can_apply(it)) {
				if(subs.apply(it)==result_t::l_applied) {
					return result_t::l_applied;
					}
				}

			if(is_termlike(it)) {
				zero(it->multiplier);
				return result_t::l_applied;
				}
			return result_t::l_no_action;
			}
		}

	if(is_nonprod_factor_in_prod(it)) {
		substitute subs(kernel, tr, args);
		if(subs.can_apply(it)) {
			if(subs.apply(it)==result_t::l_applied) {
				return result_t::l_applied;
				}
			}

		if(is_termlike(it)) {
			zero(it->multiplier);
			return result_t::l_applied;
			}
		return result_t::l_no_action;
		}

	// If we get here, we are talking about a single term, e.g.
	// ex:= x_{m};

	if(is_single_term(it)) {
		substitute subs(kernel, tr, args);
		if(subs.can_apply(it)) {
			if(subs.apply(it)==result_t::l_applied) {
				return result_t::l_applied;
				}
			}
		zero(it->multiplier);
		}

	// If we get here we have a single term for which we do not know
	// (yet) how to take a variational derivative. For instance some
	// unknown function f(a) varied wrt. a. This should spit out
	// \delta{f(a)}{\delta{a}}\delta{a} or something like that.

	throw RuntimeException("Do not yet know how to vary that expression.");
	//	std::cerr << "No idea how to vary single term " << Ex(it) << std::endl;

	return res;
	}

