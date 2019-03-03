
#include <iterator>
#include "algorithms/integrate_by_parts.hh"
#include "properties/Derivative.hh"
#include "Cleanup.hh"

using namespace cadabra;

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
				// Cleanup nested sums
				iterator tmp(sib);
				cleanup_dispatch(kernel, tr, tmp);
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

	cleanup_dispatch(kernel, tr, it);
	return ret;
	}

bool integrate_by_parts::int_and_derivative_related(iterator, iterator) const
	{
	return true;
	}

bool integrate_by_parts::derivative_acting_on_arg(iterator der_it) const
	{
	sibling_iterator arg=tr.begin(der_it);
	while(arg->is_index())
		++arg;

	Ex_comparator comp(kernel.properties);
	auto top=away_from.begin();
	if( is_in( comp.equal_subtree(arg, top), {
	Ex_comparator::match_t::subtree_match,
	Ex_comparator::match_t::match_index_less,
	Ex_comparator::match_t::match_index_greater
	} ) ) return true;
	return false;
	}

void integrate_by_parts::split_off_single_derivative(iterator, iterator der_it)
	{
	auto ni=number_of_direct_indices(der_it);
	if(ni==0 || ni==1) return;

	sibling_iterator sib=tr.begin(der_it);
	++sib;
	sibling_iterator arg=sib;
	while(arg!=tr.end(der_it) && arg->is_index())
		++arg;
	if(arg==tr.end(der_it))
		throw ConsistencyException("Derivative without argument encountered");
	auto wrap=tr.wrap(arg, str_node(der_it->name));
	while(sib!=wrap) {
		auto nxt=sib;
		++nxt;
		tr.move_before(tr.begin(wrap), sib);
		sib=nxt;
		}
	}

Algorithm::result_t integrate_by_parts::handle_term(iterator int_it, iterator& it)
	{
	// Either this is a Derivative node, in which case it is a total derivative.
	// Or this is a product, in which case we need to scan factors for a Derivative
	// and figure out whether it contains the searched-for expression.

	if(*it->name=="\\ldots") return result_t::l_no_action;

	const Derivative *dtop=kernel.properties.get<Derivative>(it);
	if(dtop) {
		if(int_and_derivative_related(int_it, it)) {
			zero(it->multiplier);
			return result_t::l_applied;
			}
		}

	prod_wrap_single_term(it);

	assert(*it->name=="\\prod");
	auto fac=tr.begin(it);
	int pos=0;
	while(fac!=tr.end(it)) {
		const Derivative *der=kernel.properties.get<Derivative>(fac);
		if(der) {
			// If this is a multiple partial derivative, we split off the
			// outermost derivative and then look at the remaining argument.
			split_off_single_derivative(int_it, fac);
			if(int_and_derivative_related(int_it, fac) && derivative_acting_on_arg(fac) ) {
				// Generate one term with the derivative acting on all
				// factors which come before the derivative node (if
				// present).  Generate another one for those factors
				// coming after the derivative (if present).

				// FIXME: this does not yet take anti-commutativity of the
				// derivative itself into account.

				if(fac==tr.begin(it) || std::next(fac)==tr.end(it)) {
					// Derivative is first or last factor in product; generate one term only.
					// Note: total derivatives have already been handled!

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
					if(std::next(from)!=to)
						from = tr.wrap(from, to, str_node("\\prod"));

					auto der_arg = tr.begin(fac);
					++der_arg;
					// This _has_ to be the argument because we have peeled off a single derivative.
					assert(der_arg->is_index()==false);

					if(der_arg==tr.end(fac))
						throw ConsistencyException("integrate_by_parts: Derivative without argument encountered.");

					tr.swap(der_arg, from);
					tr.swap(fac, der_arg);
					multiply(it->multiplier, -1);
					iterator tmp(fac);
					cleanup_dispatch(kernel, tr, tmp);
					return result_t::l_applied;
					}
				else {
					// Two terms needed.
					Ex sum("\\sum");
					auto t1prod = sum.append_child(sum.begin(), it);
					auto t2prod = sum.append_child(sum.begin(), it);

					// First term.
					sibling_iterator from=sum.begin(t1prod);
					sibling_iterator to  =from;
					to+=pos;
					if(std::next(from)!=to)
						from = tr.wrap(from, to, str_node("\\prod"));

					auto der_arg = tr.begin(to);
					while(der_arg->is_index() && der_arg!=tr.end(to))
						++der_arg;

					if(der_arg==tr.end(to))
						throw ConsistencyException("integrate_by_parts: Derivative without argument encountered.");

					tr.swap(der_arg, from);
					tr.swap(to, der_arg);
					multiply(t1prod->multiplier, -1);
					iterator tmp(to);
					cleanup_dispatch(kernel, tr, tmp);

					// Second term.
					from=sum.begin(t2prod);
					from+=pos;
					auto der=from;
					++from;
					to  =sum.end(t2prod);
					if(std::next(from)!=to)
						from = tr.wrap(from, to, str_node("\\prod"));

					der_arg = tr.begin(der);
					while(der_arg->is_index() && der_arg!=tr.end(der))
						++der_arg;

					if(der_arg==tr.end(der))
						throw ConsistencyException("integrate_by_parts: Derivative without argument encountered.");

					tr.swap(der_arg, from);
					tr.swap(der, der_arg);
					multiply(t2prod->multiplier, -1);
					tmp=der;
					cleanup_dispatch(kernel, tr, tmp);

					// Replace the original with the sum.
					it=tr.move_ontop(it, sum.begin());

					return result_t::l_applied;
					}

				//				for(auto& pat: kernel.properties.pats) {
				//					if(pat.first->name()=="Coordinate") {
				//						std::cerr << pat.second->obj << std::endl;
				//						}
				//					}
				}
			else {
				// Undo the split-off.
				iterator tmp=fac;
				cleanup_dispatch(kernel, tr, tmp);
				}
			}
		++fac;
		++pos;
		}

	return result_t::l_no_action;
	}

Ex integrate_by_parts::wrap(iterator, sibling_iterator, sibling_iterator ) const
	{
	return Ex("");
	}
