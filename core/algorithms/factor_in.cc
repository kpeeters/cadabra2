
#include "Functional.hh"
#include "algorithms/factor_in.hh"

factor_in::factor_in(const Kernel& k, Ex& tr, Ex& factors_)
	: Algorithm(k, tr), factors(factors_)
	{
	}

bool factor_in::can_apply(iterator st)
	{
	factnodes.clear();
	assert(tr.is_valid(st));
	if(*st->name=="\\sum") {
		cadabra::do_list(factors, factors.begin(), [&](Ex::iterator f) {
				factnodes.insert(Ex(f));
				return true;
				});
		return true;
		}
	else return false;
	}

hashval_t factor_in::calc_restricted_hash(iterator it) const
	{
	if(*it->name!="\\prod") return tr.calc_hash(it);

	sibling_iterator sib=tr.begin(it);
	hashval_t ret=1;
	bool first=true;
	while(sib!=tr.end(it)) { // see storage.cc for the original calc_hash
		 if(factnodes.count(Ex(sib))==0) {
			if(first) { 
				first=false;
				ret=tr.calc_hash(sib);
				}
			else { 
				ret*=17;
				ret+=tr.calc_hash(sib);
				}
			}
		++sib;
		}
	return ret;
	}

void factor_in::fill_hash_map(iterator it)
	{
	term_hash.clear();
	sibling_iterator sib=tr.begin(it);
	unsigned int terms=0;
	while(sib!=tr.end(it)) {
		term_hash.insert(std::pair<hashval_t, sibling_iterator>(calc_restricted_hash(sib), sib));
		++terms;
		++sib;
		}
	}

bool factor_in::compare_prod_nonprod(iterator prod, iterator nonprod) const
	{
	assert(*(prod->name)=="\\prod");
	assert(*(nonprod->name)!="\\prod");
	sibling_iterator it=tr.begin(prod);
	bool found=false;
	while(it!=tr.end(prod)) {
		 if(factnodes.count(Ex(it))==0) {
			 if(nonprod->name==it->name) { // FIXME: subtree_equal
				if(found) return false; // already found
				else found=true;
				}
			else return false;
			}
		++it;
		}
	if(found || (!found && factnodes.count(nonprod)!=0)) return true;
	return false;
	}

bool factor_in::compare_restricted(iterator one, iterator two) const
	{
	if(one->name==two->name) {
		if(*one->name=="\\prod") {
			sibling_iterator it1=tr.begin(one), it2=tr.begin(two);
			while(it1!=tr.end(one) && it2!=tr.end(two)) {
				 if(factnodes.count(Ex(it1))!=0) {
					++it1;
					continue;
					}
				 if(factnodes.count(Ex(it2))!=0) {
					++it2;
					continue;
					}
				iterator nxt=it1; nxt.skip_children(); ++nxt;
				if(!tr.equal(tr.begin(it1), sibling_iterator(nxt), tr.begin(it2))) 
					return false;
				++it1; ++it2;
				}
			}
		}
	else {
		if(*one->name=="\\prod" && *two->name!="\\prod") 
			return compare_prod_nonprod(one,two);
		else if(*one->name!="\\prod" && *two->name=="\\prod") 
			return compare_prod_nonprod(two,one);
		}
	return true;
	}

Algorithm::result_t factor_in::apply(iterator& it)
	{
	result_t ret=result_t::l_no_action;
	fill_hash_map(it);

	term_hash_iterator_t ht=term_hash.begin();
	while(ht!=term_hash.end()) { // loop over hash bins
		hashval_t curr=ht->first;
		term_hash_iterator_t thisbin1=ht, thisbin2=ht;
		++thisbin2;
		if(thisbin2==term_hash.end() || thisbin2->first!=thisbin1->first) { // only one term in this bin
			++ht;
			continue;
			}

		// extract the prefactor of every term in this bin.
		std::map<iterator, Ex, Ex::iterator_base_less> prefactors;
		while(thisbin1!=term_hash.end() && thisbin1->first==curr) {
			Ex prefac;
//			txtout << "doing one" << std::endl;
			prefac.set_head(str_node("\\sum"));
			if(*(thisbin1->second->name)=="\\prod") { // search for all to-be-factored-out factors 
				iterator prefacprod=prefac.append_child(prefac.begin(), str_node("\\prod", str_node::b_round));
				sibling_iterator ps=tr.begin(thisbin1->second);
				while(ps!=tr.end(thisbin1->second)) {
					 if(factnodes.count(Ex(ps))!=0) {
						iterator theterm=prefac.append_child(prefacprod, (iterator)(ps));
						theterm->fl.bracket=str_node::b_round;
						}
					++ps;
					}
				prefacprod->multiplier=thisbin1->second->multiplier;
				switch(prefac.number_of_children(prefacprod)) {
					case 0:
						prefacprod->name=name_set.insert("1").first;
						break;
					case 1:
						multiply(prefac.begin(prefacprod)->multiplier, *(prefacprod->multiplier));
						prefac.flatten(prefacprod);
						prefacprod=prefac.erase(prefacprod);
						break;
					}
				}
			else { // just insert the constant
				str_node pf("1", str_node::b_round);
				pf.multiplier=thisbin1->second->multiplier;
				prefac.append_child(prefac.begin(), pf);
				}
			prefactors[thisbin1->second]=prefac;
			++thisbin1;
			}

		// add up prefactors for terms which differ only by the prefactor
		thisbin1=ht;
		while(thisbin1!=term_hash.end() && thisbin1->first==curr) {
			thisbin2=thisbin1;
			++thisbin2;
			while(thisbin2!=term_hash.end() && thisbin2->first==curr) {
				if(compare_restricted(thisbin1->second, thisbin2->second)) {
					ret=result_t::l_applied;
//					txtout << "found match" << std::endl;
					assert(prefactors.count(thisbin1->second)>0);
					assert(prefactors.count(thisbin2->second)>0);
					iterator sumhead1=prefactors[thisbin1->second].begin();
					iterator sumhead2=prefactors[thisbin2->second].begin();
					tr.reparent(sumhead1,tr.begin(sumhead2),tr.end(sumhead2));
//					txtout << "reparented" << std::endl;
					zero((*thisbin2).second->multiplier);
					prefactors.erase(thisbin2->second);
					term_hash_iterator_t tmp=thisbin2;
					++tmp;
					term_hash.erase(thisbin2);
					thisbin2=tmp;
					}
				else ++thisbin2;
				}
			++thisbin1;
			}
		// remove old prefactors and add prefactor sums
		std::map<iterator, Ex, Ex::iterator_base_less>::iterator prefit=prefactors.begin(); 
		while(prefit!=prefactors.end()) {
			if(tr.number_of_children(prefit->second.begin())>1) { // only do this if there really is more than just one term
				sibling_iterator facit=tr.begin(prefit->first);
				while(facit!=tr.end(prefit->first)) {
					 if(factnodes.count(Ex(facit))>0)
						facit=tr.erase(facit);
					else
						++facit;
					}
				iterator inserthere=prefit->first.begin();
				if(*(prefit->first->name)!="\\prod") {
					iterator prodnode=tr.insert(prefit->first, str_node("\\prod"));
					one(prefit->first->multiplier);
					tr.append_child(prodnode, prefit->first); // FIXME: we need a 'move' 
					tr.erase(prefit->first);
					inserthere=tr.begin(prodnode);
					}
				else one(prefit->first->multiplier);
				tr.insert_subtree(inserthere, (*prefit).second.begin());
				}
			++prefit;
			}

		
		ht=thisbin1;
		}

	// Remove all terms which have zero multiplier.
	sibling_iterator one=tr.begin(it);
	while(one!=tr.end(it)) {
		if(*one->multiplier==0) 
			one=tr.erase(one);
		else if(*one->name=="\\sum" && *one->multiplier!=1) {
			sibling_iterator oneit=tr.begin(one);
			while(oneit!=tr.end(one)) {
				multiply(oneit->multiplier, *one->multiplier);
				++oneit;
				}
			one->multiplier=rat_set.insert(1).first;
			++one;
			}
		else ++one;
		}
	
	// If there is only one term left, flatten the tree.
	if(tr.number_of_children(it)==1) {
		tr.begin(it)->fl.bracket=it->fl.bracket;
		tr.begin(it)->fl.parent_rel=it->fl.parent_rel;
		tr.flatten(it);
		it=tr.erase(it);
		}
	else if(tr.number_of_children(it)==0) {
		it->multiplier=rat_set.insert(0).first;
		}


	return ret;
	}


