
#include "Storage.hh"
#include "algorithms/join_gamma.hh"
#include "Exceptions.hh"
#include "Cleanup.hh"
#include "properties/Integer.hh"

using namespace cadabra;

join_gamma::join_gamma(const Kernel& kernel, Ex& tr_, bool e, bool g)
	: Algorithm(kernel, tr_), expand(e), use_generalised_delta_(g)
	{
	}

void join_gamma::regroup_indices_(sibling_iterator gam1, sibling_iterator gam2,
                                  unsigned int i, std::vector<Ex>& r1, std::vector<Ex>& r2)
	{
	unsigned int num1=tr.number_of_children(gam1);

	unsigned int len1=0;
	unsigned int len2=0;
	sibling_iterator g1=tr.begin(gam1);
	while(len1<num1-i) {
		r1.push_back(*g1);
		++g1;
		++len1;
		}
	sibling_iterator g2=tr.begin(gam2);
	while(g2!=tr.end(gam2)) {
		if(len2>=i)
			r2.push_back(*g2);
		++g2;
		++len2;
		}
	if(i>0) {
		g2=tr.begin(gam2);
		g1=tr.end(gam1);
		--g1;
		len1=0;
		for(len1=0; len1<i; ++len1) {
			r1.push_back(*g1);
			r2.push_back(*g2);
			--g1;
			++g2;
			}
		}
	}

void join_gamma::append_prod_(const std::vector<Ex>& r1, const std::vector<Ex>& r2,
                              unsigned int num1, unsigned int num2, unsigned int i, multiplier_t mult,
                              Ex& rep, iterator loc)
	{
	Ex::iterator gamma;

	bool hasgamma     =(num1-i>0 || num2-i>0);
	bool hasdelta     =(i>0);
	bool hasmoredeltas=(i>1 && !use_generalised_delta_);

	str_node::bracket_t subsbr=gamma_bracket_;
	if((hasgamma && hasdelta) || hasmoredeltas) {
		loc=rep.append_child(loc, str_node("\\prod", gamma_bracket_, (*loc).fl.parent_rel));
		loc->multiplier=rat_set.insert(mult).first;
		subsbr=str_node::b_none;
		}

	if(num1-i>0 || num2-i>0) {
		gamma=rep.append_child(loc, str_node(*gamma_name_->name, subsbr));
		for(unsigned int j=0; j<num1-i; ++j)
			rep.append_child(gamma, r1[j].begin());
		for(unsigned int j=0; j<num2-i; ++j)
			rep.append_child(gamma, r2[j].begin()); //str_node(*r2[j].name, str_node::b_none, r2[j].fl.parent_rel));
		if(!hasdelta)
			gamma->multiplier=rat_set.insert(mult).first;
		}

	Ex::iterator delt;
	if(use_generalised_delta_ && i>0) {
		if(gm1->metric.size()==0)
			throw ConsistencyException("The gamma matrix property does not contain metric information.");

		delt=rep.append_child(loc, gm1->metric.begin());
		delt->fl.bracket=subsbr;
		tr.erase_children(delt);
		if(!hasgamma)
			delt->multiplier=rat_set.insert(mult).first;
		}

	for(unsigned int j=0; j<i; ++j) {
		if(!use_generalised_delta_) {
			if(gm1->metric.size()==0)
				throw ConsistencyException("The gamma matrix property does not contain metric information.");

			delt=rep.append_child(loc, gm1->metric.begin());
			delt->fl.bracket=subsbr;
			tr.erase_children(delt);
			}

		if(tree_exact_less(&kernel.properties, r1[j+num1-i], r2[j+num2-i]) || use_generalised_delta_) {
			rep.append_child(delt, r1[j+num1-i].begin());
			rep.append_child(delt, r2[j+num2-i].begin());
			}
		else {
			rep.append_child(delt, r2[j+num2-i].begin());
			rep.append_child(delt, r1[j+num1-i].begin());
			}
		}
	}

bool join_gamma::can_apply(iterator st)
	{
	if(*st->name=="\\prod") {
		sibling_iterator fc=tr.begin(st);
		while(fc!=tr.end(st)) {
			gm1=kernel.properties.get<GammaMatrix>(fc);
			if(gm1) {
				std::string target=get_index_set_name(begin_index(fc));
				++fc;
				if(fc!=tr.end(st)) {
					gm2=kernel.properties.get<GammaMatrix>(fc);
					if(gm2) {
						if(target==get_index_set_name(begin_index(fc))) {
							only_expand.clear();
							// FIXME: handle only expansion into single term
							//							else if(it->is_rational()) {
							//								only_expand.push_back(to_long(*it->multiplier));
							return true;
							}
						else --fc;
						}
					}
				}
			++fc;
			}
		}
	return false;
	}

Algorithm::result_t join_gamma::apply(iterator& st)
	{
	assert(*st->name=="\\prod");
	sibling_iterator gam1=tr.begin(st);
	sibling_iterator gam2;
	while(gam1!=tr.end(st)) {
		const GammaMatrix *gm1=kernel.properties.get<GammaMatrix>(gam1);
		if(gm1) {
			gamma_name_=gam1;
			gam2=gam1;
			++gam2;
			if(gam2!=tr.end(st)) {
				const GammaMatrix *gm2=kernel.properties.get<GammaMatrix>(gam2);
				if(gm2)
					break;
				}
			}
		++gam1;
		}
	if(gam1==tr.end(st)) {
		st=tr.end();
		return result_t::l_error;
		}

	gamma_bracket_=gam2->fl.bracket;

	Ex rep;
	sibling_iterator top=rep.set_head(str_node("\\sum"));

	// Figure out the dimension of the gamma matrix.
	long number_of_dimensions=-1; // i.e. not known.
	index_iterator firstind=begin_index(gam1);
	while(firstind!=end_index(gam1)) {  // select the maximum value; FIXME: be more refined...
		const Integer *ipr=kernel.properties.get<Integer>(firstind, true);
		if(ipr) {
			if(ipr->difference.begin()->is_integer()) {
				number_of_dimensions=std::max(number_of_dimensions, to_long(*ipr->difference.begin()->multiplier));
				}
			}
		else {
			number_of_dimensions=-1;
			break;
			}
		++firstind;
		}
	if(number_of_dimensions!=-1) {
		firstind=begin_index(gam2);
		while(firstind!=end_index(gam2)) {  // select the maximum value; FIXME: be more refined...
			const Integer *ipr=kernel.properties.get<Integer>(firstind, true);
			if(ipr) {
				if(ipr->difference.begin()->is_integer()) {
					number_of_dimensions=std::max(number_of_dimensions, to_long(*ipr->difference.begin()->multiplier));
					}
				}
			else {
				number_of_dimensions=-1;
				break;
				}
			++firstind;
			}
		}

	// iterators over the two index ranges
	unsigned int num1=tr.number_of_children(gam1);
	unsigned int num2=tr.number_of_children(gam2);
	for(unsigned int i=0; i<=std::min(num1, num2); ++i) {
		// Ignore gammas with more than 'd' indices.
		if(number_of_dimensions>0) {
			if(num1+num2 > number_of_dimensions+2*i) {
				continue;
				}
			}

		if(only_expand.size()!=0) {
			if(std::find(only_expand.begin(), only_expand.end(), (int)(num1+num2-2*i))==only_expand.end())
				//			if((int)(num1+num2-2*i)!=only_expand)
				continue;
			}

		std::vector<Ex> r1, r2;
		regroup_indices_(gam1, gam2, i, r1, r2);

		multiplier_t mult=(combin::fact(multiplier_t(num1))*combin::fact(multiplier_t(num2)))/
		                  (combin::fact(multiplier_t(num1-i))*combin::fact(multiplier_t(num2-i))*combin::fact(multiplier_t(i)));

		//		debugout << "join: contracting " << i << " indices..." << std::endl;
		if(!expand) {
			append_prod_(r1, r2, num1, num2, i, mult, rep, top);
			}
		else {
			combin::combinations<Ex> c1(r1);
			combin::combinations<Ex> c2(r2);
			if(num1-i>0)
				c1.sublengths.push_back(num1-i);
			if(num2-i>0)
				c2.sublengths.push_back(num2-i);
			if(use_generalised_delta_ && i>0)
				c1.sublengths.push_back(i);
			else {
				for(unsigned int k=0; k<i; ++k)
					c1.sublengths.push_back(1); // the individual \deltas, antisymmetrise 'first' group
				}
			if(i>0)
				c2.sublengths.push_back(i); // the individual \deltas, do not antisymmetrise again.

			// Collect information about which indices to write in implicit antisymmetric form.
			// FIXME: this should move into combinatorics.hh
			//			iterator it=args_begin();
			//			while(it!=args_end()) {
			//				if(*it->name=="\\comma") {
			//					sibling_iterator cst=tr.begin(it);
			//					combin::range_t asymrange1, asymrange2;
			//					while(cst!=tr.end(it)) {
			//						for(unsigned int i1=0; i1<r1.size(); ++i1) {
			//							if(subtree_exact_equal(&kernel.properties, r1[i1].begin(), cst, 0)) {
			//								asymrange1.push_back(i1);
			//								break;
			//								}
			//							}
			//						for(unsigned int i2=0; i2<r2.size(); ++i2) {
			//							if(subtree_exact_equal(&kernel.properties, r2[i2].begin(), cst, 0)) {
			//								asymrange2.push_back(i2);
			//								break;
			//								}
			//							}
			//						++cst;
			//						}
			//					c1.input_asym.push_back(asymrange1);
			//					c2.input_asym.push_back(asymrange2);
			//					}
			//				++it;
			//				}

			c1.permute();
			c2.permute();

			for(unsigned int k=0; k<c1.size(); ++k) {
				for(unsigned int l=0; l<c2.size(); ++l) {
					if(interrupted) {
						// FIXME: handle interrupts gracefully.
						//						txtout << "join interrupted while producing GammaMatrix[" << num1+num2-2*i
						//								 << "] terms." << std::endl;
						interrupted=false;
						st=tr.end();
						return result_t::l_error;
						}

					int sgn=
					   combin::ordersign(c1[k].begin(), c1[k].end(), r1.begin(), r1.end())
					   *combin::ordersign(c2[l].begin(), c2[l].end(), r2.begin(), r2.end());
					multiplier_t mul=1;
					if(use_generalised_delta_)
						mul=combin::fact(i);

					append_prod_(c1[k], c2[l], num1, num2, i,
					             multiplier_t(c1.multiplier(k))*multiplier_t(c2.multiplier(l))*sgn*mul, rep, top);
					}
				}
			}
		}

	// Finally, replace the old product by the new sum of products.
	if(rep.number_of_children(rep.begin())==0) {
		multiply(st->multiplier,0);
		return result_t::l_applied;
		}
	else if(rep.number_of_children(rep.begin())==1) {
		rep.flatten(rep.begin());
		rep.erase(rep.begin());
		}

	if(tr.number_of_children(st)>2) { // erase one gamma, replace the other one
		multiply(rep.begin()->multiplier, *gam1->multiplier);
		multiply(rep.begin()->multiplier, *gam2->multiplier);
		tr.replace(tr.erase(gam1), rep.begin());
		}
	else {
		multiply(rep.begin()->multiplier, *st->multiplier);
		st = tr.replace(st, rep.begin());
		}

	cleanup_dispatch(kernel, tr, st);
	//   cleanup_expression(tr, st);
	//   cleanup_nests(tr, st);
	return result_t::l_applied;
	}

