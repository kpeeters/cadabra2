
#include "Cleanup.hh"
#include "Functional.hh"
#include "Algorithm.hh"
#include "algorithms/collect_terms.hh"
#include "properties/Coordinate.hh"
#include "properties/SelfAntiCommuting.hh"
#include "properties/Integer.hh"
#include "properties/Diagonal.hh"
#include "properties/ExteriorDerivative.hh"
#include "properties/DifferentialForm.hh"
#include "properties/KroneckerDelta.hh"
#include "properties/Matrix.hh"
#include "properties/NumericalFlat.hh"
#include "properties/PartialDerivative.hh"
#include "properties/ImaginaryI.hh"

// #define DEBUG 1

namespace cadabra {

	void cleanup_dispatch(const Kernel& kernel, Ex& tr, Ex::iterator& it)
		{
#ifdef DEBUG
		std::cerr << "cleanup at " << *it->name << std::endl;
#endif

		// Run the cleanup as long as the expression changes.

		bool changed;
		do {
			changed=false;
			bool res=false;
			if(it->is_zero() && (tr.number_of_children(it)!=0 || *it->name!="1")) {
				cadabra::zero(it->multiplier);
				tr.erase_children(it);
				it->name=name_set.insert("1").first;
				// once we hit zero, there is nothing to simplify anymore
				break;
				}
			if(*it->name=="\\frac")                          res = cleanup_fraclike(kernel, tr, it);
			changed = changed || res;
			if(*it->name=="\\pow")                           res = cleanup_powlike(kernel, tr, it);
			changed = changed || res;
			if(*it->name=="\\prod" || *it->name=="\\wedge")  res = cleanup_productlike(kernel, tr, it);
			changed = changed || res;
			if(*it->name=="\\sum")                           res = cleanup_sumlike(kernel, tr, it);
			changed = changed || res;
			if(*it->name=="\\components")                    res = cleanup_components(kernel, tr, it);
			changed = changed || res;

			const Derivative *der = kernel.properties.get<Derivative>(it);
			if(der) {
				res = cleanup_derivative(kernel, tr, it);
				changed = changed || res;
				}
			const PartialDerivative *pder = kernel.properties.get<PartialDerivative>(it);
			if(pder) {
				res = cleanup_partialderivative(kernel, tr, it);
				changed = changed || res;
				}
			// std::cerr << "derivative " << changed << std::endl;

			const NumericalFlat *nf = kernel.properties.get<NumericalFlat>(it);
			if(nf) {
				res = cleanup_numericalflat(kernel, tr, it);
				changed = changed || res;
				}
			// std::cerr << "numerical " << changed << std::endl;

			const Diagonal *diag = kernel.properties.get<Diagonal>(it);
			if(diag) {
				res = cleanup_diagonal(kernel, tr, it);
				changed = changed || res;
				}
			// std::cerr << "diagonal " << changed << std::endl;

			//std::cerr << "Is symbol " << Ex(it) << " a KD?" << std::endl;
			const KroneckerDelta *kr = kernel.properties.get<KroneckerDelta>(it);
			if(kr) {
				//std::cerr << "Symbol " << Ex(it) << " is a KD" << std::endl;
				res = cleanup_kronecker(kernel, tr, it);
				changed = changed || res;
				}
			// std::cerr << "delta " << changed << std::endl;

			const ExteriorDerivative *ed = kernel.properties.get<ExteriorDerivative>(it);
			if(ed) {
				res = cleanup_exterior_derivative(kernel, tr, it);
				changed = changed || res;
				}

			}
		while(changed);

		//	std::cerr << Ex(it) << std::endl;
		}

	void check_index_consistency(const Kernel& k, Ex& tr, Ex::iterator it)
		{
		if(it==tr.end()) return;
		collect_terms ct(k, tr);
		ct.check_index_consistency(it);
		ct.check_degree_consistency(it); // FIXME: needs to be implemented in Algorithm.
		}

	bool cleanup_fraclike(const Kernel& k, Ex&tr, Ex::iterator& it)
		{
		auto arg=tr.begin(it);
		if(*arg->name=="\\equals") {
			// When dividing an equation by something else, divide both sides.

			auto div=arg;
			++div;

			auto lhs=tr.begin(arg);
			auto rhs=lhs;
			rhs.skip_children();
			++rhs;

			auto lhsfrac=tr.wrap(lhs, str_node("\\frac"));
			auto rhsfrac=tr.wrap(rhs, str_node("\\frac"));
			tr.append_child(lhsfrac, div);
			tr.append_child(rhsfrac, div);

			it=tr.flatten_and_erase(it);
			
			return true;
			}

		return false;
		}

	bool cleanup_powlike(const Kernel& k, Ex&tr, Ex::iterator& it)
		{
		auto arg=tr.begin(it);
		auto exp=arg;
		++exp;
		if(exp==tr.end(it)) return false;

		if(*arg->name=="1") {
			if(*arg->multiplier==0) { // 0**anything = 0
				zero(it->multiplier);
				return true;
				}
			if(*arg->multiplier==1) { // 1**anything = 1
				one(it->multiplier);
				tr.erase_children(it);
				it->name=name_set.insert("1").first;
				return true;
				}
			if(*exp->name=="1" && *exp->multiplier==-1) {    // Turn (numerical)**(-1) into a multiplier.
				multiply(it->multiplier, multiplier_t(1)/(*arg->multiplier));
				tr.erase_children(it);
				it->name = name_set.insert("1").first;
				return true;
				}
			}

		// Turn \pow{mA A}{B} with mA the multiplier for A into mA^B \pow{A}{B}
		if(exp->is_integer() && *arg->multiplier!=1 && *arg->name!="1") {
			mpz_class nw_n, nw_d;
//			std::cerr << "also doing " << arg << ", " << *arg->multiplier << "**" << *exp->multiplier << std::endl;
			long Cexp=to_long(*exp->multiplier);
			mpz_pow_ui(nw_n.get_mpz_t(), arg->multiplier->get_num().get_mpz_t(), std::abs(Cexp));
			mpz_pow_ui(nw_d.get_mpz_t(), arg->multiplier->get_den().get_mpz_t(), std::abs(Cexp));
			if(Cexp<0)
				std::swap(nw_n, nw_d);
			multiplier_t newmult=multiplier_t(nw_n, nw_d);
			it->multiplier=rat_set.insert(newmult).first;
			one(arg->multiplier);
			return true;
			}

		// Turn \pow{mult \pow{A}{B}}{C} into \pow{A}{B*C} if C is an integer.
		// A bit tricky with the multiplier of \pow{A}{B}, as that becomes mult^C
		// and can then either be absorbed into the overall multiplier, or needs
		// a second factor.
		auto ipow=tr.begin(it);
		if(*ipow->name=="\\pow") {
			auto iA=tr.begin(ipow);
			auto iB=iA;
			++iB;
			auto iC=ipow;
			++iC;
			// std::cerr << it << std::endl;
			if(iC->is_integer() || k.properties.get<Integer>(iC)) {
				if(iC->is_integer()) { // newmult = (mult)^C;
					mpz_class nw_n, nw_d;
					// std::cerr << "doing " << *ipow->multiplier << "**" << *iC->multiplier << std::endl;
					long Cexp=to_long(*iC->multiplier);
					mpz_pow_ui(nw_n.get_mpz_t(), ipow->multiplier->get_num().get_mpz_t(), std::abs(Cexp));
					mpz_pow_ui(nw_d.get_mpz_t(), ipow->multiplier->get_den().get_mpz_t(), std::abs(Cexp));
					if(Cexp<0)
						std::swap(nw_n, nw_d);
					multiplier_t newmult=multiplier_t(nw_n, nw_d);
					// std::cerr << "new multiplier " << newmult << std::endl;
					ipow->multiplier=rat_set.insert(newmult).first;
					}
				else {   // need to generate (mult)^C as a separate factor, if mult!=1.
					if(*ipow->multiplier!=1) {
						// std::cerr << "generate separate factor for " << *ipow->multiplier << "**" << iC << std::endl;
						Ex nw("\\pow");
						nw.append_child(nw.begin(), str_node("1"))->multiplier=ipow->multiplier;
						nw.append_child(nw.begin(), Ex::iterator(iC));
						tr.wrap(it, str_node("\\prod"));
						tr.insert_subtree(it, nw.begin());
						one(ipow->multiplier);
						}
					}
				Ex::iterator expprod=tr.wrap(iB, str_node("\\prod"));
				tr.move_after(iB, iC);
				it=tr.flatten_and_erase(it);
				cleanup_productlike(k, tr, expprod);
				// std::cerr << "after: " << it << std::endl;
				return true;
				}
			}

		return false;
		}

	bool cleanup_productlike(const Kernel& k, Ex&tr, Ex::iterator& it)
		{
		bool ret=false;

		assert(*it->name=="\\prod" || *it->name=="\\wedge");
		std::string nm = *it->name;

		// Flatten prod children inside this prod node.
		auto sib=tr.begin(it);
		while(sib!=tr.end(it)) {
			if(*sib->name==nm) {
				multiply(it->multiplier, *sib->multiplier);
				tr.flatten(sib);
				sib=tr.erase(sib);
				ret=true;
				}
			else ++sib;
			}

		if(tr.number_of_children(it)==1)
			if(tr.begin(it)->is_range_wildcard())
				return ret;

		ret = ret || cleanup_numericalflat(k, tr, it);

		// Turn products of ImaginaryI into -1 factors.
		if(tr.number_of_children(it)>1) {
			std::vector<Ex::sibling_iterator> fs;
			auto sib=tr.begin(it);
			while(sib!=tr.end(it)) {
				if(k.properties.get<ImaginaryI>(sib))
					fs.push_back(sib);
				++sib;
				}
			multiplier_t mult=1;
			for(size_t i=0; i<fs.size()/2; ++i) {
				tr.erase(fs[2*i]);
				tr.erase(fs[2*i+1]);
				mult*=-1;
				}
			multiply(it->multiplier, mult);
			}

		// Turn products with two adjacent identical anti-commuting siblings to zero.
		if(nm=="\\prod") {
			auto s1=tr.begin(it);
			auto s2=s1;
			++s2;
			while(s2!=tr.end(it)) {
				auto ac = k.properties.get<SelfAntiCommuting>(s1);
				if(ac) {
					if(subtree_compare(0, s1, s2)==0) {
						tr.erase_children(it);
						zero(it->multiplier);
						ret=true;
						break;
						}
					}
				++s2;
				++s1;
				}
			}

		// Turn wedge products containing two identical siblings of odd degree to zero
		// if they are not matrix objects.
		if(nm=="\\wedge") {
			auto s1=tr.begin(it);
			auto s2=s1;
			++s2;
			while(s2!=tr.end(it)) {
				if(subtree_compare(0, s1, s2)==0) {
					auto df1 = k.properties.get<DifferentialForm>(s1);
					auto df2 = k.properties.get<DifferentialForm>(s2);
					auto mat1 = k.properties.get<Matrix>(s1);
					auto mat2 = k.properties.get<Matrix>(s2);
					if(df1 && df2 && !(mat1 && mat2) ) {
						auto degree1 = df1->degree(k.properties, s1);
						auto degree2 = df2->degree(k.properties, s2);
						if(degree1.is_rational() && degree2.is_rational()) {
							long d1 = to_long(degree1.to_rational());
							long d2 = to_long(degree2.to_rational());
							if(d1==d2 && d1%2==1) {
								tr.erase_children(it);
								zero(it->multiplier);
								ret=true;
								break;
								}
							}
						}
					}
				++s2;
				++s1;
				}
			}

		// Handle edge cases where the product should collapse to a single node,
		// e.g. when we have just a single factor, or when the product vanishes.

		if(tr.number_of_children(it)==1) { // i.e. from '3*4*7*a*9'
			ret=true;
			tr.begin(it)->fl.bracket=it->fl.bracket;
			tr.begin(it)->fl.parent_rel=it->fl.parent_rel;
			tr.begin(it)->multiplier=it->multiplier;
			tr.flatten(it);
			it=tr.erase(it);
			push_down_multiplier(k, tr, it);
			}
		else if(tr.number_of_children(it)==0) {   // i.e. from '3*4*7*9'
			ret=true;
			it->name=name_set.insert("1").first;
			}


		// If any of the elements is an `\equals`, multiply the rest of the
		// terms through on both sides. If there is more than one `\equals`, throw
		// an error.

		sib=tr.begin(it);
		Ex::sibling_iterator equals_node=tr.end(it);
		while(sib!=tr.end(it)) {
			if(*sib->name=="\\equals") {
				if(equals_node!=tr.end(it))
					throw ConsistencyException("Encountered more than one equalities in a product; undefined.");
				else
					equals_node=sib;
				}
			++sib;
			}
		if(equals_node!=tr.end(it)) {
			Ex::sibling_iterator lhs=tr.begin(equals_node);
			Ex::sibling_iterator rhs=lhs;
			++rhs;

			auto lhsprod=tr.wrap(lhs, str_node("\\prod"));
			auto rhsprod=tr.wrap(rhs, str_node("\\prod"));
			sib=tr.begin(it);
			bool left=true;
			while(sib!=tr.end(it)) {
				if(sib!=equals_node) {
					if(left) {
						tr.prepend_child(lhsprod, sib);
						tr.prepend_child(rhsprod, sib);
						}
					else {
						tr.append_child(lhsprod, sib);
						tr.append_child(rhsprod, sib);
						}
					sib=tr.erase(sib);
					}
				else {
					left=false;
					++sib;
					}
				}
			Ex::iterator tmp1(lhsprod), tmp2(rhsprod);
			cleanup_dispatch(k, tr, tmp1);
			cleanup_dispatch(k, tr, tmp2);
			it=tr.flatten_and_erase(it);
			ret=true;
			}

		return ret;
		}

	bool cleanup_sumlike(const Kernel& k, Ex&tr, Ex::iterator& it)
		{
#ifdef DEBUG
		std::cerr << "cleanup_sumlike, before: " << it << std::endl;
		if(tr.number_of_children(it)==0)
			std::cerr << "zero children on sum; " << it.node << ", tree = " << tr.begin() << std::endl;
#endif
		assert(*it->name=="\\sum");
		bool ret=false;

		// Remove children which are 0
		Ex::sibling_iterator sib=tr.begin(it);
		while(sib!=tr.end(it)) {
			if(sib->is_zero()) {
				ret=true;
				sib=tr.erase(sib);
				}
			else
				++sib;
			}

		// Do not allow equalities as terms inside a sum if there are
		// other terms present as well.
		sib=tr.begin(it);
		int equalities=0;
		int nonequalities=0;
		while(sib!=tr.end(it)) {
			if(*sib->name=="\\equals") ++equalities;
			else                       ++nonequalities;
			if(equalities!=0 && nonequalities!=0) 
				throw ConsistencyException("Encountered an equality and a normal term in the same sum; not allowed.");
			++sib;
			}
		if(equalities>1) { // This is a sum of at least 2 equalities.
			// Combine all lhs and all rhs.
			auto frst=tr.begin(it);
			Ex::sibling_iterator lhs=tr.begin(frst);
			Ex::sibling_iterator rhs=lhs;
			++rhs;
			// Ensure both lhs and rhs are sums.
			if(*lhs->name!="\\sum")
				lhs=tr.wrap(lhs, str_node("\\sum"));
			if(*rhs->name!="\\sum")
				rhs=tr.wrap(rhs, str_node("\\sum"));
			Ex::sibling_iterator lhsend=tr.end(lhs);
			Ex::sibling_iterator rhsend=tr.end(rhs);
			sib=frst;
			++sib;
			while(sib!=tr.end(it)) {
            // sib is an `\equals` node
				Ex::sibling_iterator side=tr.begin(sib);
				multiply(side->multiplier, *sib->multiplier);
				tr.move_before(lhsend, side);
				side=tr.begin(sib);
				multiply(side->multiplier, *sib->multiplier);
				tr.move_before(rhsend, side);
				sib=tr.erase(sib);
				}
#ifdef DEBUG
			std::cerr << "got through equals cleanup" << std::endl;
			std::cerr << it << std::endl;
#endif
			Ex::iterator tmp1=lhs, tmp2=rhs;
			cleanup_sumlike(k, tr, tmp1);
			cleanup_sumlike(k, tr, tmp2);			
			}
		
		// Flatten sums which are supposed to be flat.
		long num=tr.number_of_children(it);
		if(num==0) {
			ret=true;
			cadabra::zero(it->multiplier);
			return ret;
			}

		if(num==1) {
			if(tr.begin(it)->is_range_wildcard())
				return ret;

			ret=true;
			multiply(tr.begin(it)->multiplier, *it->multiplier);
			tr.flatten(it);
			it=tr.erase(it);
			}
		else {
			auto facs=tr.begin(it);
			str_node::bracket_t btype_par=facs->fl.bracket;
			while(facs!=tr.end(it)) {
				if(facs->fl.bracket!=str_node::b_none) {
					btype_par=facs->fl.bracket;
					}
				++facs;
				}
			facs=tr.begin(it);
			while(facs!=tr.end(it)) {
				if(*facs->name=="\\sum") {
					auto terms=tr.begin(facs);
					auto tmp=facs;
					++tmp;
					while(terms!=tr.end(facs)) {
						multiply(terms->multiplier,*facs->multiplier);
						terms->fl.bracket=btype_par;
						++terms;
						}
					ret=true;
					tr.flatten(facs);
					tr.erase(facs);
					facs=tmp;
					}
				else ++facs;
				}
			}

		ret = ret || push_down_multiplier(k, tr, it);

#ifdef DEBUG
		std::cerr << "cleanup_sumlike, after, " << ret << ": " << it << std::endl;
#endif
		return ret;
		}

	bool push_down_multiplier(const Kernel& k, Ex& tr, Ex::iterator it)
		{
		bool ret=false;

		auto mult=*it->multiplier;
		if(mult==1)
			return ret;

		if(*it->name=="\\sum" || *it->name=="\\equals") {
			auto sib=tr.begin(it);
			while(sib!=tr.end(it)) {
				ret=true;
				multiply(sib->multiplier, mult);
				push_down_multiplier(k, tr, sib);
				++sib;
				}
			if(*it->multiplier!=1)
				ret=true;
			one(it->multiplier);
			}
		else if(*it->name=="\\components") {
			Ex::sibling_iterator sib=tr.end(it);
			--sib;
			// Examine all index value sets and push the multiplier
			// in there.
			cadabra::do_list(tr, sib, [&](Ex::iterator nd) {
				Ex::sibling_iterator val=tr.begin(nd);
				++val;
				if(mult!=1) {
					ret=true;
					multiply(val->multiplier, mult);
					}
				// Need to evaluate it; just putting it in '||' may lead to the compiler not evaluating it if
				// 'ret' is already true!
				bool tmp = push_down_multiplier(k, tr, val);
				ret = ret || tmp;
				return true;
				});
			if(*it->multiplier!=1)
				ret=true;
			one(it->multiplier);
			}

		return ret;
		}

	bool cleanup_components(const Kernel& k, Ex&tr, Ex::iterator& it)
		{
		assert(*it->name=="\\components");

		bool ret=push_down_multiplier(k, tr, it);

		// If this component node has no free indices, get rid of all
		// the baggage and turn into a normal expression.

		// std::cerr << "components cleanup: " << Ex(it) << std::endl;

		auto comma=tr.begin(it);
		if(*comma->name=="\\comma") { // If the first child is \comma, there are no indices: scalar.
			if(tr.number_of_children(comma)==0) {
				// Totally empty component node, can happen after an
				// evaluate with no rules matching.
				zero(it->multiplier);
				ret=true;
				return ret;
				}
			ret=true;
			// std::cerr << "components node for a scalar" << std::endl;
			tr.flatten(comma);     // unwrap comma
			comma=tr.erase(comma); // erase comma
			tr.flatten(comma);     // unwrap equals
			comma=tr.erase(comma); // erase equals
			comma=tr.erase(comma); // remove empty comma for index values
			tr.flatten(it); // remove components node
			it=tr.erase(it);
			// std::cerr << Ex(it) << std::endl;
			}
		else {
			while(comma!=tr.end(it)) {
				if(*comma->name=="\\comma") {
					if(tr.number_of_children(comma)==0) {
						ret=true;
						zero(it->multiplier);
						}
					// Still check if there is a component value for which the index
					// values exactly match the index names. In that case, replace
					// the entire components node with the component value.
					auto equals   = tr.begin(comma);
					while(equals != tr.end(comma)) {
						auto valcomma = tr.begin(equals);
						auto valindices=tr.begin(valcomma);
						auto expindices=tr.begin(it);
						Ex_comparator comp(k.properties);
						bool foundmatch=true;
						while(valindices!=tr.end(valcomma)) {
							auto match = comp.equal_subtree(valindices, expindices, Ex_comparator::useprops_t::not_at_top, true);
							if(! (match==Ex_comparator::match_t::node_match || match==Ex_comparator::match_t::subtree_match)) {
								foundmatch=false;
								break;
								}
							++expindices;
							++valindices;
							}
						if(foundmatch) {
							// Yep, we can unwrap this component and replace it with the
							// single value.
							auto erase=tr.begin(it);
							while(erase!=comma)    // erase indices from \components
								erase=tr.erase(erase);
							auto eit=tr.begin(comma);
							while(eit!=tr.end(comma)) { // erase all component values which we do not need.
								if(eit==equals) ++eit;
								else            eit=tr.erase(eit);
								}
							tr.flatten(comma);     // unwrap comma
							comma=tr.erase(comma); // erase comma
							tr.flatten(comma);     // unwrap equals
							comma=tr.erase(comma); // erase equals
							comma=tr.erase(comma); // remove comma node (plus its children) for index values
							tr.flatten(it); // remove components node
							it=tr.erase(it);
							return true;
							}
						++equals;
						}
					// None of the index value sets match the index names. If the index names are
					// coordinates, this means that the value of this component is zero.
					auto expindices=tr.begin(it);
					bool all_coordinates=true;
					while(*expindices->name!="\\comma") {
						if(expindices->is_integer()==false && k.properties.get<Coordinate>(expindices, true)==0) {
							all_coordinates=false;
							break;
							}
						++expindices;
						}
					if(all_coordinates) {
						zero(it->multiplier);
						return true;
						}
					return ret;
					}
				++comma;
				}
			// Anonymous tensor with all components vanishing.
			ret=true;
			zero(it->multiplier);
			}

		return ret;
		}

	bool cleanup_partialderivative(const Kernel&, Ex& tr, Ex::iterator& it)
		{
		// Nested derivatives with the same name should be flattened, but
		// only if both the outer derivative and the inner derivative have
		// an index (otherwise D(D(A)) becomes D(A) which is wrong).

		// Find first non-index child.

		bool ret=false;

		Ex::sibling_iterator sib=tr.begin(it);
		if(sib==tr.end(it)) return ret;

		while(sib->is_index()) {
			++sib;
			if(sib==tr.end(it)) {
				zero(it->multiplier);
				return true;
				}
			if(sib==tr.end(it))
				throw ConsistencyException("Encountered PartialDerivative object without argument on which to act.");
			}

		// FIXME: this ignores that derivatives can have functional child
		// nodes which are interpreted as 'object wrt. with derivative should be taken'.

		if(it->name == sib->name) {
			if(Algorithm::number_of_direct_indices(it)>0 && Algorithm::number_of_direct_indices(sib)>0) {
				multiply(it->multiplier, *sib->multiplier);
				tr.flatten(sib);
				tr.erase(sib);
				ret=true;
				}
			}

		return ret;
		}

	bool cleanup_derivative(const Kernel& k, Ex& tr, Ex::iterator& it)
		{
		bool ret=false;

		if(Algorithm::number_of_direct_indices(it) == tr.number_of_children(it)) {
			// This is a derivative acting on nothing, always occurs
			// when all constants have been moved out.
			zero(it->multiplier);
			ret=true;
			return ret;
			}

		auto sib=tr.begin(it);
		while(sib!=tr.end(it)) {
			if(sib->fl.parent_rel==str_node::p_none) {
				if(*sib->name=="\\equals") {
					// FIXME: this should probably be taken out for generalisation.
					auto lhs = tr.begin(sib);
					auto rhs = lhs;
					++rhs;

					auto lhswrap = tr.wrap(lhs, *it);
					auto rhswrap = tr.wrap(rhs, *it);
					multiply(lhswrap->multiplier, *it->multiplier);
					multiply(rhswrap->multiplier, *it->multiplier);

					auto sib2=tr.begin(it);
					while(sib2!=tr.end(it)) {
						if(sib2!=sib) {
							tr.insert_subtree(lhs, sib2);
							tr.insert_subtree(rhs, sib2);
							sib2=tr.erase(sib2);
							}
						else ++sib2;
						}

					it=tr.flatten(it);
					it=tr.erase(it);

					Ex::iterator tmp1(lhswrap), tmp2(rhswrap);
					cleanup_dispatch(k, tr, tmp1);
					cleanup_dispatch(k, tr, tmp2);

					ret=true;
					break;
					}
				}
			++sib;
			}

		return ret;
		}

	bool cleanup_numericalflat(const Kernel&, Ex& tr, Ex::iterator& it)
		{
		bool ret=false;

		// Collect all multipliers and remove resulting '1' nodes.
		auto facs=tr.begin(it);
		multiplier_t factor=1;
		while(facs!=tr.end(it)) {
			if(facs->is_index()==false) { // Do not collect the number in e.g. \partial_{4}{A}.
				factor*=*facs->multiplier;
				if(facs->is_rational()) {
					multiplier_t tmp; // FIXME: there is a bug in gmp which means we have to put init on next line.
					tmp=(*facs->name).c_str();
					ret=true;
					factor*=tmp;
					facs=tr.erase(facs);
					if(facs==tr.end())
						facs=tr.end(it);
					}
				else {
					if(*facs->multiplier!=1)
						ret=true;
					one(facs->multiplier);
					++facs;
					}
				}
			else ++facs;
			}
		if(factor!=1)
			ret=true;

		multiply(it->multiplier,factor);
		return ret;
		}

	bool cleanup_diagonal(const Kernel&, Ex& tr, Ex::iterator& it)
		{
		bool ret=false;

		if(tr.number_of_children(it)!=2) return ret;

		auto c1=tr.begin(it);
		auto c2(c1);
		++c2;

		if(c1->is_rational() && c2->is_rational())
			if(c1->multiplier != c2->multiplier) {
				ret=true;
				zero(it->multiplier);
				}
		if(!(c1->is_rational() && c2->is_rational())) {
			if(subtree_compare(0, c1, c2)!=0) {
				ret=true;
				zero(it->multiplier);
				}
			}

		return ret;
		}

	bool cleanup_kronecker(const Kernel&, Ex& tr, Ex::iterator& it)
		{
		bool ret=false;

		if(tr.number_of_children(it)!=2) return ret;

		auto c1=tr.begin(it);
		auto c2(c1);
		++c2;

		if(c1->is_rational() && c2->is_rational()) {
			if(c1->multiplier != c2->multiplier) {
				ret=true;
				zero(it->multiplier);
				}
			else {
				//			::one(it->multiplier);
				tr.erase_children(it);
				ret=true;
				it->name=name_set.insert("1").first;
				}
			}

		return ret;
		}

	bool cleanup_exterior_derivative(const Kernel& k, Ex& tr, Ex::iterator& it)
		{
		// FIXME: could have this act on a sum as well.
		if(tr.number_of_children(it)==1) {
			auto sib=tr.begin(it);
			const ExteriorDerivative *ed1=k.properties.get<ExteriorDerivative>(it);
			const ExteriorDerivative *ed2=k.properties.get<ExteriorDerivative>(sib);
			if(ed1==ed2) {
				zero(it->multiplier);
				return true;
				}
			}
		return false;
		}


	void cleanup_dispatch_deep(const Kernel& k, Ex& tr, dispatcher_t dispatch)
		{
		Ex::iterator top=tr.begin();
		cleanup_dispatch_deep(k, tr, top, dispatch);
		}

	void cleanup_dispatch_deep(const Kernel& k, Ex& tr, Ex::iterator&, dispatcher_t dispatch)
		{
		// Cleanup the entire tree starting from the deepest nodes and
		// working upwards.

		// This duplicates work of Algorithm::apply, but we want to have an
		// independent cleanup unit which does not rely on things we may
		// want to change in Algorithm::apply in the future, and we do not
		// want to make recursive calls into that function either. And it is
		// simple enough anyway.

		//	do_subtree(tr, top, [&dispatch, &tr, &k](Ex::iterator it) {
		//			dispatch(k, tr, it);
		//			return it;
		//			});

		Ex::post_order_iterator it=tr.begin();
		it.descend_all();
		while(it!=tr.end()) {
			Ex::post_order_iterator next=it;
			++next;
			Ex::iterator tmp=it;
			dispatch(k, tr, tmp);
			it=next;
			}

		}

	}
