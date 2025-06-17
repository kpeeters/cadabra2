#include <algorithm>
#ifdef HAS_TBB
#include <execution>
#endif
#include "algorithms/collect_terms.hh"

using namespace cadabra;

collect_terms::collect_terms(const Kernel& k, Ex& tr)
	: Algorithm(k, tr)
	{
	}

bool collect_terms::can_apply(iterator st)
	{
	assert(tr.is_valid(st));
	if(*st->name=="\\sum") return true;
	return false;
	}

void collect_terms::fill_hash_map(iterator it)
	{
	term_hash.clear();
#ifdef HAS_TBB
	size_t num = tr.number_of_children(it);
	std::vector<std::pair<hashval_t, sibling_iterator>> hash_pairs(num);
	std::vector<sibling_iterator>                       iterators(num);
	sibling_iterator sib=tr.begin(it);
	for(size_t i=0; i<num; ++i) {
		iterators[i]=sib;
		++sib;
		}
	
	std::transform(std::execution::par_unseq,
						iterators.begin(), iterators.end(),
						hash_pairs.begin(),
						[this, &iterators, &it](sibling_iterator term_it)
							{
							auto hv = tr.calc_hash(term_it);
							return std::make_pair(hv, term_it);
							}
						);

	term_hash = term_hash_t(hash_pairs.begin(), hash_pairs.end());
#else
	sibling_iterator sib=tr.begin(it);
	sibling_iterator end=tr.end(it);
	while(sib!=end) {
		term_hash.insert(std::pair<hashval_t, sibling_iterator>(tr.calc_hash(sib), sib));
		++sib;
		}
#endif
	}

void collect_terms::remove_zeroed_terms(sibling_iterator from, sibling_iterator to)
	{
	// Remove all terms which have zero multiplier.
	sibling_iterator one=from;
	while(one!=to) {
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
	}

Algorithm::result_t collect_terms::apply(iterator& st)
	{
	assert(tr.is_valid(st));
	assert(*st->name=="\\sum");
	fill_hash_map(st);
	result_t res=collect_from_hash_map();
	remove_zeroed_terms(tr.begin(st), tr.end(st));

	// If there is only one term left, flatten the tree.
	if(tr.number_of_children(st)==1) {
		// tr.print_recursive_treeform(std::cerr, st);
		tr.begin(st)->fl.bracket=st->fl.bracket;
		tr.begin(st)->fl.parent_rel=st->fl.parent_rel;
		tr.flatten(st);
		st=tr.erase(st);
		// tr.print_recursive_treeform(std::cerr, st);
		// We may have to propagate the multiplier up the tree to make it consistent.
		pushup_multiplier(st);
		}
	else if(tr.number_of_children(st)==0) {
		//		zero(st->multiplier);
		node_zero(st);
		}
	return res;
	}

Algorithm::result_t collect_terms::collect_from_hash_map()
	{
	result_t res=result_t::l_no_action;
	term_hash_iterator_t ht=term_hash.begin();
	while(ht!=term_hash.end()) {
		hashval_t curr=ht->first;  // hash value of the current set of terms
		term_hash_iterator_t thisbin1=ht, thisbin2;
		while(thisbin1!=term_hash.end() && thisbin1->first==curr) {
			thisbin2=thisbin1;
			++thisbin2;
			while(thisbin2!=term_hash.end() && thisbin2->first==curr) {
				if(subtree_exact_equal(&kernel.properties, (*thisbin1).second, (*thisbin2).second, -2, true, 0, true)) {
					res=result_t::l_applied;
					add((*thisbin1).second->multiplier, *((*thisbin2).second->multiplier));
					zero((*thisbin2).second->multiplier);
					term_hash_iterator_t tmp=thisbin2;
					++tmp;
					term_hash.erase(thisbin2);
					thisbin2=tmp;
					}
				else ++thisbin2;
				}
			++thisbin1;
			}
		ht=thisbin1;
		}

	return res;
	}
