
#include <boost/utility.hpp>
#include "algorithms/integrate_by_parts.hh"
#include "properties/Derivative.hh"
#include "Cleanup.hh"

integrate_by_parts::integrate_by_parts(const Kernel& k, Ex& tr, Ex& af)
	: Algorithm(k, tr), away_from(af)
	{
	}

bool integrate_by_parts::can_apply(iterator st)
	{
	if(*st->name=="\\int") return true;
	return false;
	}

Algorithm::result_t integrate_by_parts::apply(iterator& it)
	{
	result_t ret=result_t::l_no_action;

	auto sib=tr.begin(it);
	while(sib!=tr.end(it)) {
		if(sib->fl.parent_rel==str_node::p_none) {
			// Have found the integrand.
			if(*sib->name=="\\sum") {
				auto term=tr.begin(sib);
				while(term!=tr.end(sib)) {
					iterator ti(term);
					++term;
					auto res=handle_term(it, ti);
					if(res==result_t::l_applied) {
						ret=res;
						cleanup_dispatch(kernel, tr, ti);
						}
					}
				}
			else {
				iterator ti(sib);
				ret=handle_term(it, ti);
				if(ret==result_t::l_applied)
					cleanup_dispatch(kernel, tr, ti);
				}
			break;
			}
		++sib;
		}

	return ret;
	}

bool integrate_by_parts::int_and_derivative_related(iterator int_it, iterator der_it) const
	{
	return true;
	}

bool integrate_by_parts::derivative_acting_on_arg(iterator der_it) const
	{
	auto stop=der_it;
	stop.skip_children();
	++stop;
	
	++der_it;
	Ex_comparator comp(kernel.properties);
	auto top=away_from.begin(away_from.begin());
	while(der_it!=stop) {
		if( comp.equal_subtree(der_it, top)==Ex_comparator::match_t::subtree_match) return true;
		++der_it;
		}
	return false;
	}

Algorithm::result_t integrate_by_parts::handle_term(iterator int_it, iterator& it)
	{
	// Either this is a Derivative node, in which case it is a total derivative.
	// Or this is a product, in which case we need to scan factors for a Derivative
	// and figure out whether it contains the searched-for expression.

	const Derivative *dtop=kernel.properties.get<Derivative>(it);
	if(dtop) {
		if(int_and_derivative_related(int_it, it)) {
			zero(it->multiplier);
			return result_t::l_applied;
			}
		}

	assert(*it->name=="\\prod");
	auto fac=tr.begin(it);
	while(fac!=tr.end(it)) {
		const Derivative *der=kernel.properties.get<Derivative>(fac);
		if(der) {
			if(int_and_derivative_related(int_it, fac) && derivative_acting_on_arg(fac) ) {
				// Generate one term with the derivative acting on all
				// factors which come before the derivative node (if
				// present).  Generate another one for those factors
				// coming after the derivative (if present).  

				// FIXME: this does not yet take anti-commutativity of the
				// derivative itself into account.

				if(fac==tr.begin(it) || boost::next(fac)==tr.end(it)) {
					// Derivative is first or last factor in product; generate one term only.
					
					sibling_iterator from, to;
					if(fac==tr.begin(it)) {
						from=fac;
						++from;
						to=tr.end(it);
						}
					else {
						from=tr.begin(it);
						to=fac;
						}
					if(boost::next(from)!=to)
						from = tr.wrap(from, to, str_node("\\prod"));

					auto der_arg = tr.begin(fac);
					while(der_arg->is_index() && der_arg!=tr.end(fac))
						++der_arg;
					if(der_arg==tr.end(fac)) return result_t::l_no_action; // no idea what to do here
					tr.swap(der_arg, from);
					tr.swap(fac, der_arg);
					multiply(it->multiplier, -1);
					return result_t::l_applied;
					}
				else {
					// Two terms needed.
					Ex sum("\\sum");
					auto ofac=tr.begin(it);
					while(ofac!=tr.end(it)) {
						if(ofac!=fac) {
							Ex prod(it);
							
							}
						++ofac;
						}
					}

//				for(auto& pat: kernel.properties.pats) {
//					if(pat.first->name()=="Coordinate") {
//						std::cerr << pat.second->obj << std::endl;
//						}
//					}
				}
			}
		++fac;
		}

	return result_t::l_no_action;
	}

Ex integrate_by_parts::wrap(iterator prod, sibling_iterator from, sibling_iterator to) const
	{
	return Ex("");
	}
