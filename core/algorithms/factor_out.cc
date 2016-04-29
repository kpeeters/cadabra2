
#include "Functional.hh"
#include "algorithms/factor_out.hh"
#include <map>

factor_out::factor_out(const Kernel& k, Ex& e, Ex& args)
	: Algorithm(k, e)
	{
	cadabra::do_list(args, args.begin(), [&](Ex::iterator arg) {
			to_factor_out.push_back(Ex(arg));
			return true;
			}
		);
	}

/// Check if the expression is a sum with more than one term
bool factor_out::can_apply(iterator st)
	{
	if(*st->name=="\\sum")  return true;
	return false;
	}

/* Tries to factor out the given factors from the expression.
 
   The algorithm goes through each term in the sum and extracts all factors
   from it. These are a stored in a list of factors along with the term they came from.
   Each different factor is then reinserted back into the expression but multiplied by
   the sum of all the terms that factor came from.
 
   Note that factors are extracted on both the left and right to allow for 
   non-commutative factoring, and A, B, and A * B are treated as seperate factors.
*/

Algorithm::result_t factor_out::apply(iterator& it)
	{
	result_t result=result_t::l_no_action;

	// For each term with factors, collector is used to store the factors
	// and the rest of the term they came from.
	typedef std::multimap<Ex, sibling_iterator> collector_t; // FIXME: original this had Ex_is_less as comparator
	collector_t collector;

	sibling_iterator st=tr.begin(it);
	while(st!=tr.end(it)) { // Loop through each term in the sum
		Ex left_factors("\\prod"), right_factors("\\prod"); // Where we will store extracted factors
		
		if(*st->name=="\\prod") {
			extract_factors(st, true, left_factors);
			extract_factors(st, false, right_factors);
			
			std::cerr << "left: " << left_factors << std::endl;
			std::cerr << "right: " << right_factors << std::endl;

			// The factors are not extracted in any particular order.
			// We want to put them in some canonical order so that equal factors are grouped.
			order_factors(st, left_factors);
			order_factors(st, right_factors);
			
			// If removing factors reduced the term to something simple, modify the
			// tree to show this.
			if(tr.number_of_children(st)==0) {
				rset_t::iterator mtmp=st->multiplier;
				node_one(st);
				st->multiplier=mtmp;
				}
			else if(tr.number_of_children(st)==1) {
				multiply(tr.begin(st)->multiplier, *st->multiplier);
				tr.flatten(st);
				st=tr.erase(st);
				}
			}
		else {
			// If the term is a factor we are looking for then we pull the factor
			// out and replace it by a 1.
			for(unsigned int tfo=0; tfo < to_factor_out.size(); ++tfo) {
				Ex_comparator comparator(kernel.properties);
				if(comparator.equal_subtree(static_cast<iterator>(st),
				                            to_factor_out[tfo].begin())==Ex_comparator::subtree_match) {
					iterator tmp=left_factors.append_child(left_factors.begin(), static_cast<iterator>(st));
					one(tmp->multiplier);
					rset_t::iterator mtmp=st->multiplier;
					node_one(st);
					st->multiplier=mtmp;
					break;
					}
				}
			}
		
		// left_factors and right_factors are now products of the factors 
		// that were found in the current term.
		if (left_factors.number_of_children(left_factors.begin()) > 0 ||
		    right_factors.number_of_children(right_factors.begin()) > 0) {
			Ex sum;
			// Encode as left_factors + right_factors so that we can still use the
			// existing method with an Ex as the key. This is very hacky, and
			// might cause problems if A + B is regarded as the same tree as B + A.
			sum.set_head(str_node("\\sum"));
			sum.append_child(sum.begin(), left_factors.begin());
			sum.append_child(sum.begin(), right_factors.begin());
			collector.insert(std::make_pair(sum, st));
			}
		
		++st;
		}

	if(collector.size()==0) return result_t::l_no_action;
	
	for(auto& c: collector) {
		std::cerr << c.first << std::endl;
		}
	std::cerr << Ex(it) << std::endl;

	// The expression is currently in a weird state - we have pulled out all of the factors from each
	// term that they appeared, but not added them back in anywhere. Since we now have a multimap with
	// keys corresponding to each type of factor (A, B, AB, etc) and members corresponding to the rest
	// of the term that they came from, we can go through and collect together terms that have the same
	// factor and reinsert them into the tree.

	collector_t::iterator ci=collector.begin();
	Ex oldkey = (*ci).first;
	while(ci!=collector.end()) {
		Ex term("\\prod"); // Create the * in (factor * (a + b + c)).
		const Ex thiskey=(*ci).first;
		
		// Extract left_factors and right_factors from how they were stored
		// as left_factors + right_factors
		sibling_iterator sum_iter = thiskey.begin(thiskey.begin());
		iterator left_factors = sum_iter;
		iterator right_factors = ++sum_iter;

		std::cerr << "handling " << Ex(left_factors) << std::endl;
		
		// Insert left factors on the left.
		if (thiskey.number_of_children(left_factors) > 0)
			term.reparent(term.begin(), left_factors);
		
		std::cerr << "term now " << term << std::endl;

		sibling_iterator sumit=term.append_child(term.begin(),str_node("\\sum"));
		size_t terms=0;
		Ex_is_equivalent cmp(kernel.properties);
		// Go through each term with the same factor and include it in the sum.
		// We also remove it from the tree since it will be added back in in its
		// factorised form.
		while(ci!=collector.end() && cmp((*ci).first, oldkey)) {
			term.append_child(sumit, (*ci).second);
			tr.erase((*ci).second);
			++terms;
			++ci;
			}
		
		std::cerr << "term then " << term << std::endl;

		// Insert right factors on the right.
		if (thiskey.number_of_children(right_factors) > 0)
			term.reparent(term.begin(), right_factors);
		
		if(terms>1) result=result_t::l_applied;
		
		if(terms==1) { // a sum with only one child
			term.flatten(sumit);
			term.erase(sumit);
			}
		
		std::cerr << "term later " << term << std::endl;
		std::cerr << "orig then  " << Ex(it) << std::endl;

		// Put our factor * (a + b + ...) piece back into the tree.
//		tr.insert_subtree(tr.begin(it), term.begin());
		tr.append_child(it, term.begin());
		
		std::cerr << "the tree is now " << Ex(it) << std::endl;

		if(ci==collector.end())
			break;
		oldkey=(*ci).first;
		}

	if(tr.number_of_children(it)==1) { // the sum has been reduced to a single term now
		tr.flatten(it);
		it=tr.erase(it);
		}
	
	return result;
	}

/* Extracts all possible factors from a product node.
 
   extract_factors takes a product node in the main expression tree and
   extracts any factors which are present, respecting anti and non-commutivity.
   The factors are removed from the product node and placed into another
   provided product node. The factors will either be extracted to the left if
   left_to_right is true or from the right if not.
  
   @param product a sibling_iterator pointing to the node of the expression 
   tree to extract the factors from. This should be a product node.
   @param left_to_right If true factors are extracted to the left,
   if false they are extracted to the right.
   @param collector an Ex with a product node as the head. Extracted
   factors are inserted into this as children.
*/
void factor_out::extract_factors(sibling_iterator product, bool left_to_right, Ex& collector) 
	{
	if (tr.number_of_children(product) == 0)
		return;
	
	sibling_iterator begin, end;
	if (left_to_right) {
		// Set begin the to the left most child, end to the right most.
		begin = tr.begin(product);
		end = begin; end += tr.number_of_siblings(begin);
		}
	else {
		// Set begin the to the right most child, end to the left most.
		end = tr.begin(product);
		begin = end; begin += tr.number_of_siblings(end);
		}
	
	// factor_signs stores the sign that would need to be picked up by each factor if it
	// was to be moved through the terms we have encountered so far.
	// A value of 0 means the factor can't be moved through the terms.
	std::vector<int> factor_signs (to_factor_out.size(), 1);
	
	bool beginning_preserved = true;
	bool first_element = true;
	sibling_iterator current_term = begin;
	do { // Loop through from begin to end, including both begin and end.
		if (!first_element) {
			if (left_to_right)
				current_term++;
			else
				current_term--;
			}
		first_element = false;
		std::cerr << "considering " << Ex(current_term) << std::endl;
		
		bool factor_removed = false;
		
		// Check to see if the current term is one of the factors we are looking for.
		for(unsigned int tfo=0; tfo<to_factor_out.size(); ++tfo) {
			Ex_comparator comparator(kernel.properties);
			if(comparator.equal_subtree(static_cast<iterator>(current_term),
			                            to_factor_out[tfo].begin()) == Ex_comparator::subtree_match) {
				if (factor_signs[tfo] != 0) {
					// Put the term into the collector and remove it from the tree, picking up
					// a sign if necessary.
					if (left_to_right)
						collector.append_child(collector.begin(), static_cast<iterator>(current_term));
					else
						collector.prepend_child(collector.begin(), static_cast<iterator>(current_term));
					multiply(product->multiplier, factor_signs[tfo]);
					tr.erase(current_term);
					factor_removed = true;
					// FIXME: V2, the code below needs checking
//					if (!beginning_preserved)
//						// We've gone past a term which we didn't factor out
//						result=result_t::l_applied;
					}
				break;
				}
			}
		
		if (!factor_removed) {
			// Strictly speaking the beginning may still be preserved, so we don't
			// set expression_modified to true until we pull out another factor, in which
			// case it definitely isn't.
			beginning_preserved = false;
			
			// If the term wasn't moved to the front as a factor then we have to consider the
			// commutivity properties of it with other factors that we may later move through it.
			Ex_comparator comparator(kernel.properties);
			for(unsigned int tfo=0; tfo < to_factor_out.size(); ++tfo) {
				int stc = subtree_compare(&kernel.properties, to_factor_out[tfo].begin(), current_term);
				int sign = comparator.can_swap(to_factor_out[tfo].begin(), current_term, stc);
				factor_signs[tfo] *= sign;
				}
			}
		
		// The above algorithm has each term compared to each factor twice. Once to check if the
		// the term is a factor, and if not, once to check its commutitivity properties with the factors.
		// It would be nice to only have one loop, but I think we need to know we're not going to move
		// the term before we start working out the commutitivity properties.
		} while(current_term != end);
	}

/* Takes an product of factors and tries to put its term
   in the same order the factors were provided in.
 
   order_factors takes a product node of factors and orders the terms in it as closely as
   possible to the order of factors given by the user. Anti and non-commutivity
   are respected, and the sign of the term the factors came from will be updated
   as necessary.
  
   @param product a sibling_iterator corresponding to the product node that
   the factors came from. This may have its sign changed.
   @param factors an Ex with a product node as head, and the collected
   factors as children. This is what is put in order.
   @param first_unordered_term a sibling_iterator pointing to the term in
   factors from which the ordering should start. This is mostly used 
   internally by the function and omitting this argument will order from the 
   beginning.
*/
void factor_out::order_factors(sibling_iterator product, Ex& factors, sibling_iterator first_unordered_term) 
	{
	bool passed_non_commuting_term = false;
	sibling_iterator first_non_commuting_term;
	
	for (unsigned int tfo=0; tfo < to_factor_out.size(); ++tfo) {
		// Try to pull out each factor in turn from the remaining unordered part of the
		// expression.
		int sign = 1;
		for(sibling_iterator psi=first_unordered_term; psi != factors.end(factors.begin()); ++psi) {
			Ex_comparator comparator(kernel.properties);
			if(comparator.equal_subtree(static_cast<iterator>(psi),
			                            to_factor_out[tfo].begin()) == Ex_comparator::subtree_match) {
				if (sign != 0) {
					if (psi == first_unordered_term) {
						++first_unordered_term; // This is safe if nodes are linked rather than indexed.
						}
					else {
						factors.move_before(first_unordered_term, psi);
						multiply(product->multiplier, sign);
						}
					}
				}
			else {
				int stc = subtree_compare(&kernel.properties, to_factor_out[tfo].begin(), psi);
				sign *= comparator.can_swap(to_factor_out[tfo].begin(), psi, stc);
				if (sign == 0) {
					if (!passed_non_commuting_term) {
						first_non_commuting_term = psi;
						}
					else {
						if (factors.index(psi) < factors.index(first_non_commuting_term))
							first_non_commuting_term = psi;
						}
					passed_non_commuting_term = true;
					break; // We're not going to be able to extract this factor.
					}
				}
			}
		}
	
	// A non commuting term will stop us bringing anything that doesn't
	// commute with it to the front. Thus we need to order all the terms 
	// after it again.
	if (passed_non_commuting_term) {
		sibling_iterator next_term = ++first_non_commuting_term;
		if (next_term != factors.end(factors.begin()))
			order_factors(product, factors, next_term);
		}
	}

void factor_out::order_factors(sibling_iterator product, Ex& factors) 
	{
	sibling_iterator first_unordered_term = factors.begin(factors.begin());
	order_factors(product, factors, first_unordered_term);
	}
