
#include "Props.hh"
#include "Compare.hh"
#include "Cleanup.hh"
#include "algorithms/collect_factors.hh"
#include "algorithms/collect_terms.hh"
#include "properties/Symbol.hh"

using namespace cadabra;

collect_factors::collect_factors(const Kernel& k, Ex& e)
	: Algorithm(k, e)
	{
	}

bool collect_factors::can_apply(iterator it)
	{
	if(*it->name=="\\prod") return true;
	return false;
	}

// The hash map is such that all objects which are equal have to sit in the same
// bin, but objects in the same bin do not necessarily all have to be equal.
void collect_factors::fill_hash_map(iterator it)
	{
	factor_hash.clear();
	sibling_iterator sib=tr.begin(it);
	unsigned int factors=0;
	while(sib!=tr.end(it)) {
		sibling_iterator chsib=tr.begin(sib);
		bool dontcollect=false;
		while(chsib!=tr.end(sib)) {
			const Symbol     *smb=kernel.properties.get<Symbol>(chsib, true);
			// std::cerr << chsib << ": " << smb << std::endl;
			 if((chsib->fl.parent_rel==str_node::p_sub || chsib->fl.parent_rel==str_node::p_super) &&
				 chsib->is_rational()==false && smb==0) {
				dontcollect=true;
				break;
				}
			++chsib;
			}
		if(!dontcollect) {
			if(*sib->name=="\\pow") 
				factor_hash.insert(std::pair<hashval_t, sibling_iterator>(tr.calc_hash(tr.begin(sib)), tr.begin(sib)));
			else
				factor_hash.insert(std::pair<hashval_t, sibling_iterator>(tr.calc_hash(sib), sib));
			++factors;
			}
		++sib;
		}
	}

Algorithm::result_t collect_factors::apply(iterator& st)
	{
	assert(tr.is_valid(st));
	assert(*st->name=="\\prod");
	result_t res=result_t::l_no_action;

	Ex_comparator comp(kernel.properties);

	fill_hash_map(st);
	factor_hash_iterator_t ht=factor_hash.begin();
	while(ht!=factor_hash.end()) {
		hashval_t curr=ht->first;  // hash value of the current set of terms
		factor_hash_iterator_t thisbin1=ht, thisbin2;
		while(thisbin1!=factor_hash.end() && thisbin1->first==curr) {
			thisbin2=thisbin1;
			++thisbin2;
			Ex expsum;
			iterator expsumit=expsum.set_head(str_node("\\sum"));
			// add the exponent of the first element in this hash bin
			if(*(tr.parent((*thisbin1).second)->name)=="\\pow") {
				sibling_iterator powch=tr.parent((*thisbin1).second).begin();
				++powch;
				iterator newch= expsum.append_child(expsumit, iterator(powch));
				newch->fl.bracket=str_node::b_round;
				}
			else {
				expsum.append_child(expsumit, str_node("1", str_node::b_round));
				}
			assert(*((*thisbin1).second->multiplier)==1);
			// find the other, identical factors
			while(thisbin2!=factor_hash.end() && thisbin2->first==curr) {
				if(subtree_exact_equal(&kernel.properties, (*thisbin1).second, (*thisbin2).second)) {
					// only do something if this factor can be moved to the other one
					iterator objnode1=(*thisbin1).second;
					iterator objnode2=(*thisbin2).second;
					if(*tr.parent(objnode1)->name=="\\pow") objnode1=tr.parent(objnode1);
					if(*tr.parent(objnode2)->name=="\\pow") objnode2=tr.parent(objnode2);
					if(comp.can_move_adjacent(st, objnode1, objnode2)) {
						// all clear
						assert(*((*thisbin2).second->multiplier)==1);
						res=result_t::l_applied;
						if(*(tr.parent((*thisbin2).second)->name)=="\\pow") {
							sibling_iterator powch=tr.parent((*thisbin2).second).begin();
							++powch;
							iterator newch=expsum.append_child(expsumit, iterator(powch));
							newch->fl.bracket=str_node::b_round;
							}
						else {
							expsum.append_child(expsumit, str_node("1", str_node::b_round));
							}
						factor_hash_iterator_t tmp=thisbin2;
						++tmp;
						if(*(tr.parent((*thisbin2).second)->name)=="\\pow")
							tr.erase(tr.parent((*thisbin2).second));
						else
							tr.erase((*thisbin2).second);
						factor_hash.erase(thisbin2);
						thisbin2=tmp;
						res=result_t::l_applied;
						}
					else ++thisbin2;
					}
				else ++thisbin2;
				}
			// make the modification to the tree
			if(expsum.number_of_children(expsum.begin())>1) {
				iterator top=expsum.begin();
				cleanup_dispatch(kernel,expsum, top);
//				cleanup_nests_below(expsum, expsum.begin());
				if(! (expsum.begin()->is_identity()) ) {
					collect_terms ct(kernel, expsum);
					iterator tp=expsum.begin();
					ct.apply(tp);

					iterator inserthere=thisbin1->second;
					if(*(tr.parent(inserthere)->name)=="\\pow")
						inserthere=tr.parent(inserthere);
					if(expsum.begin()->is_rational() && (expsum.begin()->is_identity() ||
																	 expsum.begin()->is_zero() ) ) {
						if(*(inserthere->name)=="\\pow") {
							tr.flatten(inserthere);
							inserthere=tr.erase(inserthere);
							sibling_iterator nxt=inserthere;
							++nxt;
							tr.erase(nxt);
							}
						if(expsum.begin()->is_zero()) {
							rset_t::iterator rem=inserthere->multiplier;
							node_one(inserthere);
							inserthere->multiplier=rem;
							}
						}
					else {
						Ex repl;
						repl.set_head(str_node("\\pow"));
						repl.append_child(repl.begin(), iterator((*thisbin1).second));
						repl.append_child(repl.begin(), expsum.begin());
						if(*(inserthere->name)!="\\pow") {
							inserthere=(*thisbin1).second;
							}
						tr.insert_subtree(inserthere, repl.begin());
						tr.erase(inserthere);
						}
					}
				}
//			else txtout << "only one left" << std::endl;
			++thisbin1;
			}
		ht=thisbin1;
		}
	cleanup_dispatch(kernel, tr, st);
	return res;
	}

