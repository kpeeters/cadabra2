
#include <vector>
#include <set>
#include <numeric>

#include "Cleanup.hh"
#include "DisplayTerminal.hh"
#include "Functional.hh"
#include "Linear.hh"

#include "algorithms/meld.hh"

#include "properties/Coordinate.hh"
#include "properties/Derivative.hh"
#include "properties/Diagonal.hh"
#include "properties/ImplicitIndex.hh"
#include "properties/RiemannTensor.hh"
#include "properties/PartialDerivative.hh"
#include "properties/Symbol.hh"
#include "properties/Trace.hh"
#include "properties/Traceless.hh"
#include "properties/TableauBase.hh"
#include "properties/SelfNonCommuting.hh"
#include "properties/NonCommuting.hh"

using namespace cadabra;

meld::meld(const Kernel& kernel, Ex& ex, bool project_as_sum)
	: Algorithm(kernel, ex)
	, index_map(kernel)
	, project_as_sum(project_as_sum)
{

}

meld::~meld()
{

}

bool meld::can_apply(iterator it)
{
	return
		can_apply_diagonals(it) ||
		can_apply_traceless(it) ||
		can_apply_cycle_traces(it) ||
		can_apply_tableaux(it);
}

meld::result_t meld::apply(iterator& it)
{
	result_t res = result_t::l_no_action;

	if (can_apply_diagonals(it) && apply_diagonals(it)) {
		res = result_t::l_applied;
		cleanup_dispatch(kernel, tr, it);
	}
	if (can_apply_traceless(it) && apply_traceless(it)) {
		res = result_t::l_applied;
		cleanup_dispatch(kernel, tr, it);
	}
	if (can_apply_cycle_traces(it) && apply_cycle_traces(it)) {
		res = result_t::l_applied;
		cleanup_dispatch(kernel, tr, it);
	}
	//if (can_apply_side_relations(it) && apply_side_relations(it)) {
	//	res = result_t::l_applied;
	//	cleanup_dispatch(kernel, tr, it);
	//}
	if (can_apply_tableaux(it) && apply_tableaux(it)) {
		res = result_t::l_applied;
		cleanup_dispatch(kernel, tr, it);
	}

	return res;
}


// *_diagonals
// Remove Diagonal objects with numerical indices which are not all the same.

bool meld::can_apply_diagonals(iterator it)
{
	auto diagonal = kernel.properties.get<Diagonal>(it);
	return diagonal != nullptr;
}

bool meld::apply_diagonals(iterator it)
{
	
	assert(kernel.properties.get<Diagonal>(it) != nullptr);
	index_iterator indit = begin_index(it);
	if (indit->is_rational()) {
		index_iterator indit2 = indit;
		++indit2;
		while (indit2 != end_index(it)) {
			if (indit2->is_rational() == false)
				break;
			if (indit2->multiplier != indit->multiplier) {
				zero(it->multiplier);
				return true;
			}
			++indit2;
		}
	}
	return false;
}


// *_traceless
// // Remove any traces of traceless tensors.

bool meld::can_apply_traceless(iterator it)
{
	auto traceless = kernel.properties.get<Traceless>(it);
	return traceless != nullptr;
}

bool meld::apply_traceless(iterator it)
{
	
	const Traceless* trl = kernel.properties.get<Traceless>(it);
	unsigned int ihits = 0;
	tree_exact_less_mod_prel_obj comp(&kernel.properties);
	std::set<Ex, tree_exact_less_mod_prel_obj> countmap(comp);
	index_iterator indit = begin_index(it);
	while (indit != end_index(it)) {
		bool incremented_now = false;
		auto ind = kernel.properties.get<Indices>(indit, true);
		if (ind) {
			// The indexs need to be in the set for which the object is
			// traceless (if specified, otherwise accept all).
			if (trl->index_set_names.find(ind->set_name) != trl->index_set_names.end() || trl->index_set_names.size() == 0) {
				incremented_now = true;
				++ihits;
			}
		}
		else incremented_now = true;
		// Having no name is treated as having the right name
		if (countmap.find(Ex(indit)) == countmap.end()) {
			countmap.insert(Ex(indit));
		}
		else if (incremented_now) {
			zero(it->multiplier);
			return true;
		}
		++indit;
	}
	iterator parent = it;
	if (tr.number_of_children(it) == 1 && !tr.is_head(it)) parent = tr.parent(it);
	const Trace* trace = kernel.properties.get<Trace>(parent);
	if (trace) {
		int tmp;
		auto impi = kernel.properties.get_with_pattern<ImplicitIndex>(it, tmp, "");
		if (impi.first->explicit_form.size() > 0) {
			// Does the explicit form have two more indices of the right type?
			Ex::iterator eform = impi.first->explicit_form.begin();
			unsigned int ehits = 0;
			indit = begin_index(eform);
			while (indit != end_index(eform)) {
				auto ind = kernel.properties.get<Indices>(indit, true);
				if (trl->index_set_names.find(ind->set_name) != trl->index_set_names.end() && ind->set_name == trace->index_set_name) ++ehits;
				if (ehits - ihits > 1) {
					zero(it->multiplier);
					return true;
				}
				++indit;
			}
		}
	}
	return false;
}


// *_tableaux

bool meld::can_apply_tableaux(iterator it)
{
	// This node can be a sum, but the rest of the tree must be strictly monomial. Also
	// helps if there is an index lying around somewhere
	bool found_index = false;
	for (Ex::iterator beg = it.begin(), end = it.end(); beg != end; ++beg) {
		if (*beg->name == "\\sum" || *beg->name == "\\equals" || *beg->name == "\\comma")
			return false;
		if (beg->is_index()) {
			found_index = true;
			beg.skip_children();
		}
	}

	return found_index;
}

bool meld::apply_tableaux(iterator it)
{
	if (*it->name == "\\equals") {
		bool res = false;
		Ex::sibling_iterator side = it.begin();
		res |= apply_tableaux(side);
		++side;
		res |= apply_tableaux(side);
		return res;
	}

	using namespace boost::numeric::ublas;
	using matrix_type = matrix<mpq_class>;
	using vector_type = vector<mpq_class>;

	bool applied = false;

	// Collect terms which have a matching structure (modulo index positions) into groups
	std::vector<std::vector<ProjectedTerm>> patterns;
	for (const auto& term : split_it(it, "\\sum")) {
		ProjectedTerm projected_term(kernel, index_map, tr, term);
		if (projected_term.ident.size() == 0)
			continue;
		bool found = false;
		for (auto& pattern : patterns) {
			if (pattern[0].compare(kernel, projected_term)) {
				found = true;
				pattern.push_back(projected_term);
				break;
			}
		}
		if (!found)
			patterns.emplace_back(1, std::move(projected_term));
	}

	// Apply to each pattern group in turn
	for (auto& terms : patterns) {
		ScopedProgressGroup group(pm,
			"Melding terms of form " + ex_to_string<DisplayTerminal>(kernel, terms[0].tensor),
			terms.size());

		// Initialize the linear solver; 'coeffs' is a square matrix of YP coefficients which grows
		// every time we encounter a linearly-independent term. 'mapping' is a map between matrix
		// rows and YP terms. 'adjforms' is a list of the complete decompositions (one for each column
		// in 'coeffs') which we need to keep to ensure the solution we get by solving for 'coeffs', which
		// does not contain the coefficients for every term in the YP, is an actual solution 
		linear::Solver<mpq_class> solver;
		matrix_type coeffs;
		std::vector<Adjform> mapping;

		// Calculate the symmetrizers for this pattern group. If we are symmetrizing as a sum, then each
		// new term in the sum is marked by the 'independent' flag being set
		auto tabs = collect_tableaux(terms[0].tensor);
		std::vector<symmetrizer_t> symmetrizers;
		bool is_zero = collect_symmetries(tabs, symmetrizers);

		if (is_zero) {
			// The term is identically zero due to its tableaux, delete all and move
			// onto next pattern
			for (auto& term : terms) {
				node_zero(term.it);
				applied = true;
			}
			continue;
		}

		// Go through all terms in this pattern group one at a time
		for (size_t term_idx = 0; term_idx < terms.size(); ++term_idx) {
			group.progress();
			auto& term = terms[term_idx];
			symmetrize(term, symmetrizers);

			if (term.projection.empty()) {
				// Empty adjform means that the term is identically equal to 0
				node_zero(term.it);
				terms.erase(terms.begin() + term_idx);
				--term_idx;
				applied = true;
			}
			else {
				// We need to try and express the current YP as a linear combination of previous YPs
				// by solving "coeffs * x = y"
				vector_type x, y;
				bool has_solution = false;

				if (coeffs.size1() > 0) {
					y.resize(coeffs.size1());
					for (size_t i = 0; i < mapping.size(); ++i)
						y(i) = term.projection.get(mapping[i]);
					x = solver.solve(y);
					has_solution = true;

					// x is guaranteed to be a solution as the 'coeffs' matrix is square and the columns
					// are linearly dependent. To check whether it is actually a solution, go back over
					// all the adjforms and ensure that the equation holds for each term
					// To do this we keep track of iterators into each YP we have calculated. Different YPs
					// will contain different terms (as they are a sparse storage), but as they are
					// held in a std::map the terms are already sorted, so when constructing the vector
					// which holds iterators into each YP we find which one has the smallest term.
					// Then starting with this term we go through each iterator; if it points to this term
					// then we accumulate the value * x[i] and increment the iterator, otherwise we skip
					// over it. If the YP we just calculated contains this term then we check to make sure
					// the value in the right hand equals this total, otherwise we check to make sure the total
					// was 0. If we find a mismatch we set has_solution to false, otherwise we continue doing
					// this until all the iterators have expired

					std::vector<ProjectedAdjform::const_iterator> lhs_its;
					ProjectedAdjform::const_iterator rhs_it = term.projection.begin();
					Adjform cur_term = rhs_it->first;

					// Populate the lhs_its vector and find the first (i.e. smallest) term
					for (size_t i = 0; i < term_idx; ++i) {
						auto it = terms[i].projection.begin();
						if (it->first < cur_term)
							cur_term = it->first;
						lhs_its.push_back(it);
					}

					// Keep on iterating while there are unexpired iterators
					size_t n_finished = 0;
					while (n_finished < lhs_its.size()) {
						// Calculate the sum on the left hand side. We simultaneously calculate the
						// next term which we will examine by checking every time we increment an
						// iterator if it is smaller than next_term (which we initialize to be the
						// largest possible)
						Adjform next_term;
						next_term.push_coordinate(std::numeric_limits<Adjform::value_type>::max());
						mpq_class sum = 0;
						for (size_t i = 0; i < term_idx; ++i) {
							if (lhs_its[i] != terms[i].projection.end() && lhs_its[i]->first == cur_term) {
								sum += x(i) * lhs_its[i]->second;
								++lhs_its[i];
								if (lhs_its[i] == terms[i].projection.end())
									++n_finished;
							}
							if (lhs_its[i] != terms[i].projection.end() && lhs_its[i]->first < next_term)
								next_term = lhs_its[i]->first;
						}

						// Calculate the sum on the right hand side 
						mpq_class rhs_sum;
						if (rhs_it == term.projection.end() || rhs_it->first != cur_term) {
							rhs_sum = 0;
						}
						else {
							rhs_sum = rhs_it->second;
							++rhs_it;
						}

						// Early return if there is a mismatch
						if (sum != rhs_sum) {
							has_solution = false;
							break;
						}

						// See if next smallest term is from the YP we just calculated
						if (rhs_it != term.projection.end() && rhs_it->first < next_term)
							next_term = rhs_it->first;
						cur_term = next_term;
					}

					// If all the LHS iterators have expired, but there are still non-zero terms
					// on the RHS (i.e. the iterator isn't expired) then this is a mismatch
					if (rhs_it != term.projection.end())
						has_solution = false;
				}

				if (has_solution) {
					// If there is a solution, we add contributions from the current term to the
					// scalar parts of all the other terms and set their 'changed' flag to true.
					// Then zero and erase the current node; this is the only change to the actual
					// tree we make right now, we will make the changes to the other nodes once we
					// have accumulated all the contributions
					for (size_t i = 0; i < term_idx; ++i) {
						if (x(i) != 0) {
							terms[i].changed = true;
							Ex::iterator scalar_head = term.scalar.begin();
							for (Ex::sibling_iterator beg = scalar_head.begin(), end = scalar_head.end(); beg != end; ++beg) {
								auto new_term = terms[i].scalar.append_child(terms[i].scalar.begin(), (Ex::iterator)beg);
								multiply(new_term->multiplier, x(i) * (*scalar_head->multiplier));
							}
						}
					}
					applied = true;
					node_zero(term.it);
					terms.erase(terms.begin() + term_idx);
					--term_idx;
				}
				else {
					// Expand the dimensions of the matrix by 1
					coeffs.resize(coeffs.size1() + 1, coeffs.size2() + 1);

					// Find a representative term for the YP we just calculated to add to the matrix, i.e.
					// a term which isn't already in the 'mapping'. Once we find one, we fill in the bottom
					// row (the coefficient this term has in the previously calculated YPs), the right
					// hand column (the coefficient in of each term in 'mapping' in the YP we just
					// calculated) and the bottom right element (the representative term in the new YP).
					bool found = false;
					for (const auto& kv : term.projection) {
						auto pos = std::find(mapping.begin(), mapping.end(), kv.first);
						if (pos == mapping.end()) {
							// Fill in bottom row
							for (size_t i = 0; i < term_idx; ++i)
								coeffs(coeffs.size1() - 1, i) = terms[i].projection.get(kv.first);
							// Fill in the righthand column
							for (size_t i = 0; i < mapping.size(); ++i)
								coeffs(i, coeffs.size2() - 1) = term.projection.get(mapping[i]);
							// Fill in the bottom right element
							coeffs(coeffs.size1() - 1, coeffs.size2() - 1) = term.projection.get(kv.first);
							if (solver.factorize(coeffs)) {
								mapping.push_back(kv.first);
								found = true;
								break;
							}
						}
					}

					// Shouldn't ever happen...if this error does get thrown then probably need a new way
					// to calculate the representative terms
					if (!found)
						throw std::runtime_error("Could not find a suitable element to add to the matrix");
				} // if (has_solution) {} else {}
			} // if (term.projection.empty()) {} else {}
		} //for (size_t term_idx = 0; term_idx < terms.size(); ++term_idx)

		// Replace any nodes which have the 'changed' flag
		for (auto& term : terms) {
			if (term.changed) {
				// Replace the node with a product of the scalar and tensor parts, then cleanup
				tr.erase_children(term.it);
				term.it = tr.replace(term.it, str_node("\\prod"));
				tr.append_child(term.it, term.scalar.begin());
				tr.append_child(term.it, term.tensor.begin());
				cleanup_dispatch(kernel, tr, term.it);
			}
		}
	} // for (auto& terms : patterns)

	return applied;
}

meld::ProjectedTerm::ProjectedTerm(const Kernel& kernel, IndexMap& index_map, Ex& ex, Ex::iterator it)
	: scalar("\\sum")
	, tensor("\\prod")
	, it(it)
	, changed(false)
{
	// Split the term up into a scalar part and a tensor part. The scalar part always starts
	// with a \\sum node, as contributions will be added to it during the melding process,
	// so we start by adding a \\prod node which will collect the scalar factors.
	auto scalar_head = scalar.append_child(scalar.begin(), str_node("\\prod"));

	// If the object is not a product, then it either a single scalar object or a single
	// tensor object; detect which it is and move onto the appropriate part.
	if (*it->name != "\\prod") {
		iter_indices term_indices(kernel.properties, it);
		if (term_indices.size() == 0) {
			scalar.append_child(scalar_head, it);
		}
		else {
			// Put the tensor on the tensor node, but move the multiplier over to
			// the scalar part
			auto term = tensor.append_child(tensor.begin(), it);
			auto factor = scalar.append_child(scalar_head, str_node("1"));
			multiply(factor->multiplier, *it->multiplier);
			one(term->multiplier);
		}
	}
	else {
		// Object is a product of multiple terms.
		// Loop through all terms in the product. If they have indices, then see if they
		// can commute through the tensor bits in front of it to join other scalar terms
		// out the front. Otherwise it will have to stay in the the tensor part of the
		// expression
		Ex_comparator comp(kernel.properties);
		// Position of the last scalar value in the expression. Start this off with a
		// sentinel "null iterator" so that we know we haven't met any scalar terms yet
		Ex::iterator last_scalar(0);
		for (Ex::sibling_iterator beg = it.begin(), end = it.end(); beg != end; ++beg) {
			// Determine if it is scalar or tensor. We decide this by assuming it is a
			// scalar, and then iterating through its indices checking for one which isn't
			// a coordinate, symbol or integer. If we find a 'real' index, we know that it
			// is a tensor and can stop checking the indices.
			bool is_scalar = true;
			iter_indices term_indices(kernel.properties, beg);
			size_t n_indices = term_indices.size();
			for (const auto& idx : term_indices) {
//				Ex::iterator idx = *term_indices.begin();
				auto symb = kernel.properties.get<Symbol>(idx, true);
				auto coord = kernel.properties.get<Coordinate>(idx, true);
				bool is_index = !(symb || coord || idx->is_integer());
				if (is_index) {
					is_scalar = false;
					break;
				}
			}
			// If it is a scalar term, then attempt to commute it through the expression
			// to join the rest of the scalar terms. If it can't commute through, then
			// mark it as a tensor --- this ensures that it won't get moved anywhere.
			if (is_scalar) {
				if (last_scalar == Ex::iterator(0))
					is_scalar = comp.can_move_to_front(ex, it, beg);
				else
					is_scalar = comp.can_move_adjacent(it, last_scalar, beg);
			}

			// Move scalar terms onto the scalar node, and tensor (including non-
			// commuting tensors) onto the tensor node
			if (is_scalar) {
				auto term = scalar.append_child(scalar_head, (Ex::iterator)beg);
				multiply(term->multiplier, *it->multiplier);
				last_scalar = beg;
			}
			else {
				tensor.append_child(tensor.begin(), (Ex::iterator)beg);
			}
		}
	}

	// If we had no scalar components, then create a numeric constant to
	// hold the overall factor
	if (scalar_head.number_of_children() == 0) {
		auto term = scalar.append_child(scalar_head, str_node("1"));
		multiply(term->multiplier, *it->multiplier);
	}

	// Flatten/cleanup the expressions
	Ex::iterator tensor_head = tensor.begin();
	cleanup_dispatch(kernel, scalar, scalar_head);
	cleanup_dispatch(kernel, tensor, tensor_head);

	// Calculate the index structure of the tensor part.
	auto ibeg = index_iterator::begin(kernel.properties, tensor.begin());
	auto iend = index_iterator::end(kernel.properties, tensor.begin());
	ident = Adjform(ibeg, iend, index_map, kernel);
}

// Return 'true' if the tensor parts are identical up to index structure.
bool meld::ProjectedTerm::compare(const Kernel& kernel, const ProjectedTerm& other)
{
	auto head1 = tensor.begin(), head2 = other.tensor.begin();
	if (head1->name != head2->name)
		return false;

	auto separated_by_derivative = [&kernel](const Ex& ex, Ex::iterator a, Ex::iterator b) {
		// Climb the tree until we meet, returning true if we find a derivative along the way
		Ex::iterator lca = ex.lowest_common_ancestor(a, b);
		while (a != lca || b != lca) {
			// Check for partial derivative
			if (kernel.properties.get<Derivative>(a))
				return true;
			if (kernel.properties.get<Derivative>(b))
				return true;
			// Move nodes up a level
			if (a != lca)
				a = ex.parent(a);
			if (b != lca)
				b = ex.parent(b);
		}
		return false;
	};

	std::set<Ex::iterator> dummies1, dummies2;
	Ex_comparator comp(kernel.properties);

	Ex::iterator beg1 = head1.begin(), end1 = head1.end();
	Ex::iterator beg2 = head2.begin(), end2 = head2.end();
	for (; beg1 != end1 && beg2 != end2; ++beg1, ++beg2) {
		auto match = comp.equal_subtree(beg1, beg2);
		if (match == Ex_comparator::match_t::subtree_match) {
			// Whole subtree is a match, skip children and continue
			beg1.skip_children();
			beg2.skip_children();
			continue;
		}
		if (beg1->name == beg2->name && beg1->fl.parent_rel == beg2->fl.parent_rel) {
			// Nodes are the same, continue but don't skip children
			continue;
		}

		// No match is ok if the index structure is the same. Let's check that they
		// are both indices
		if (!beg1->is_index() || !beg2->is_index())
			return false;

		// We will not need to dig further into the tree if these match
		beg1.skip_children();
		beg2.skip_children();

		// We begin by checking if they are coordinates,
		// symbols or integers in which case they must match exactly
		bool int1 = beg1->is_integer();
		bool int2 = beg2->is_integer();
		bool coord1 = kernel.properties.get<Coordinate>(beg1, true);
		bool coord2 = kernel.properties.get<Coordinate>(beg2, true);
		bool sym1 = kernel.properties.get<Symbol>(beg1, true);
		bool sym2 = kernel.properties.get<Symbol>(beg2, true);

		if ((int1 && int2) || (coord1 && coord2) || (sym1 && sym2))
			return true;
		if ((int1 != int2) || (coord1 != coord2) || (sym1 != sym2))
			return false;

		// Okay, we will treat these as indices of some sort now. First we check for
		// Indices property to check the sets
		auto iprop1 = kernel.properties.get<Indices>(beg1);
		auto iprop2 = kernel.properties.get<Indices>(beg2);

		// If neither is in a set then they are both free and that is fine
		if (!iprop1 && !iprop2)
			continue;

		// If one is a set but the other isn't then its a mismatch
		if ((bool)iprop1 != (bool)iprop2)
			return false;

		// If they are both in sets but they're different sets then its a mismatch
		if (iprop1->set_name != iprop2->set_name)
			return false;

		// Ok - they're both in the same set. If they are at the same height then that
		// is fine
		if (beg1->fl.parent_rel == beg2->fl.parent_rel)
			continue;

		// They are at different heights. If they're free then thats fine
		if (iprop1->position_type == Indices::position_t::free)
			continue;

		// If they are independent then thats a mismatch
		if (iprop1->position_type == Indices::position_t::independent)
			return false;

		// Fixed is okay if they are in a dummy pair which isn't separated by a derivative
		if (iprop1->position_type == Indices::position_t::fixed) {
			bool beg1isdummy = false, beg2isdummy = false;
			if (dummies1.find(beg1) != dummies1.end()) {
				beg1isdummy = true;
				dummies1.erase(dummies1.find(beg1));
			}
			if (dummies2.find(beg2) != dummies2.end()) {
				beg2isdummy = true;
				dummies2.erase(dummies2.find(beg2));
			}
			// Secondly we iterate through the rest of the tree to check for a match
			if (!beg1isdummy) {
				Ex::iterator search = beg1;
				search.skip_children();
				++search;
				while (search != end1) {
					comp.clear();
					if (comp.equal_subtree(beg1, search, Ex_comparator::useprops_t::never, true) == Ex_comparator::match_t::subtree_match) {
						// Found dummy, first we check whether it is separated by a derivative
						if (!separated_by_derivative(tensor, beg1, search)) {
							// Valid dummy, add this iterator to dummies so we can find it later
							dummies1.insert(search);
							beg1isdummy = true;
						}
					}
					if (search->is_index())
						search.skip_children();
					++search;
				}
			}
			if (!beg2isdummy) {
				Ex::iterator search = beg2;
				search.skip_children();
				++search;
				while (search != end2) {
					if (comp.equal_subtree(beg2, search) == Ex_comparator::match_t::subtree_match) {
						// Found dummy, first we check whether it is separated by a derivative
						Ex::iterator tmp;
						if (!separated_by_derivative(other.tensor, beg2, search)) {
							// Valid dummy, add this iterator to dummies so we can find it later
							dummies2.insert(search);
							beg2isdummy = true;
						}
					}
					if (search->is_index())
						search.skip_children();
					++search;
				}
			}
			// In case we've forgotten what we were meant to be doing here; the two indices have different
			// heights but this is ok if they are both in valid dummy pairs; so we return false if that
			// is not the case
			if (!beg1isdummy || !beg2isdummy)
				return false;
		}
	}

	// One of the iterators has expired, check that both have
	return beg1 == end1 && beg2 == end2;
}

std::vector<meld::tab_t> meld::collect_tableaux(Ex& ex) const
{
	std::vector<tab_t> tabs;
	size_t total_indices = 0;
	for (const auto& term : split_it(ex.begin(), "\\prod")) {
		auto tb = kernel.properties.get<TableauBase>(term);
		if (tb) {
			if (project_as_sum && !tabs.empty()) 
				throw std::runtime_error("meld cannot project_as_sum the product of tensors with non-trivial tableau shapes");

			size_t n_tabs = tb->size(kernel.properties, ex, term);
			for (size_t i = 0; i < n_tabs; ++i) {
				auto tab = tb->get_tab(kernel.properties, ex, term, i);
				for (auto& cell : tab)
					cell += total_indices;
				tabs.push_back(std::move(tab));
			}

			// Are we a derivative of a Riemann tensor?
			if (n_tabs == 1) {
				Ex::iterator child = term;
				size_t depth = 0;
				while (kernel.properties.get<Derivative>(child)) {
					child = child.begin();
					++child;
					++depth;
				}
				if (kernel.properties.get<RiemannTensor>(child)) {
					// Append indices to top row of Riemann tableau
					for (size_t k = 0; k < depth; ++k) {
						tabs.back().add_box(0, total_indices + k);
					}
				}
			}
		}
		iter_indices indices(kernel.properties, term);
		total_indices += indices.size();
	}

	return tabs;
}

bool meld::collect_symmetries(const std::vector<tab_t>& tabs, std::vector<symmetrizer_t>& symmetrizers) const
{
	if (project_as_sum)
		return collect_symmetries_as_sum(tabs, symmetrizers);
	else
		return collect_symmetries_as_product(tabs, symmetrizers);
}

bool meld::collect_symmetries_as_product(const std::vector<tab_t>& tabs, std::vector<symmetrizer_t>& symmetrizers) const
{
	// We collect all the symmetrizers and antisymmerizers into a list
	// to end up with e.g.
	//   S(01) A(02) S(34) A(57) A(68) S(56) S(78)
	// We then apply operations to this list by commuting symmetrizers
	// through each other and collecting any symmetrizers which
	// cancel each other

	for (const auto& tab : tabs) {
		for (size_t col = 0; col < tab.row_size(0); ++col) {
			if (tab.column_size(col) > 1) {
				symmetrizer_t sym(true, true);
				sym.indices.assign(tab.begin_column(col), tab.end_column(col));
				std::sort(sym.indices.begin(), sym.indices.end());
				symmetrizers.push_back(std::move(sym));
			}
		}
		for (size_t row = 0; row < tab.number_of_rows(); ++row) {
			if (tab.row_size(row) > 1) {
				symmetrizer_t sym(false, true);
				sym.indices.assign(tab.begin_row(row), tab.end_row(row));
				std::sort(sym.indices.begin(), sym.indices.end());
				symmetrizers.push_back(std::move(sym));
			}
		}
	}

	// For each symmetrizer i, try and commute it though the symmetrizers to the right and left
	// of it to try and find simplifications
	for (size_t i = 0; i < symmetrizers.size(); ++i) {
		auto& lhs = symmetrizers[i].indices;
		// Commute right
		for (size_t j = i + 1; j < symmetrizers.size(); ++j) {
			// Calculate intersection and union of the two terms
			auto& rhs = symmetrizers[j].indices;
			std::vector<size_t> uni, inter;
			std::set_union(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), std::back_inserter(uni));
			std::set_intersection(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), std::back_inserter(inter));
			bool can_commute = inter.empty();

			if (symmetrizers[i].antisymmetric == symmetrizers[j].antisymmetric) {
				// Both symmetric/antisymmetric: can be combined if one is a subset of the other.
				if (lhs == uni || rhs == uni) {
					lhs = uni;
					symmetrizers.erase(symmetrizers.begin() + j);
					--j;
				}
			}
			else {
				// One is symmetric and the other antisymmetric: if they overlap by more than one index
				// then the whole projection is identically zero
				if (inter.size() > 1) {
					return true;
				}
			}

			// If these two terms do not commute then move lhs on
			if (!can_commute) {
				symmetrizers[i].independent = false;
				break;
			}
		}

		// Commute left
		for (size_t j = i - 1; j != (size_t)-1; --j) {
			// Calculate intersection and union of the two terms
			auto& rhs = symmetrizers[j].indices;
			std::vector<size_t> uni, inter;
			std::set_union(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), std::back_inserter(uni));
			std::set_intersection(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), std::back_inserter(inter));
			bool can_commute = inter.empty();

			if (symmetrizers[i].antisymmetric == symmetrizers[j].antisymmetric) {
				// Both symmetric/antisymmetric: can be combined if one is a subset of the other.
				if (lhs == uni || rhs == uni) {
					lhs = uni;
					symmetrizers.erase(symmetrizers.begin() + j);
					++j;
					--i;
				}
			}
			else {
				// One is symmetric and the other antisymmetric: if they overlap by more than one index
				// then the whole projection is identically zero
				if (inter.size() > 1) {
					return true;
				}
			}

			// If these two terms do not commute then move lhs on
			if (!can_commute) {
				symmetrizers[i].independent = false;
				break;
			}
		}
	}

	return false;
}


bool meld::collect_symmetries_as_sum(const std::vector<tab_t>& tabs, std::vector<symmetrizer_t>& symmetrizers) const
{
	auto reduce_tab = [](tab_t tab) {
		// Get the row with the biggest element
		size_t n_cells = 0;
		size_t greatest_row = 0;
		int greatest_elem = -1;
		for (size_t row = 0; row < tab.number_of_rows(); ++row) {
			n_cells += tab.row_size(row);
			int back = (int)tab(row, tab.row_size(row) - 1);
			if (back > greatest_elem) {
				greatest_elem = back;
				greatest_row = row;
			}
		}

		tab.remove_box(greatest_row);
		return tab;
	};

	auto is_trivial = [](const tab_t& tab) {
		return std::distance(tab.begin(), tab.end()) <= 2;
	};

	std::vector<mpz_class> norms;
	for (const auto& tab : tabs) {
		// Check tableau is row-standard
		for (size_t row = 0; row < tab.number_of_rows(); ++row) {
			int prev = -1;
			for (auto beg = tab.begin_row(row), end = tab.end_row(row); beg != end; ++beg) {
				int next = *beg;
				if (next < prev)
					throw ConsistencyException("Trying to symmetrize non-standard tableau as sum");
				prev = next;
			}
		}
		// Check tableau is column-standard
		for (size_t col = 0; col < tab.row_size(0); ++col) {
			int prev = -1;
			for (auto beg = tab.begin_column(col), end = tab.end_column(col); beg != end; ++beg) {
				int next = *beg;
				if (next < prev)
					throw ConsistencyException("Trying to symmetrize non-standard tableau as sum");
				prev = next;
			}
		}

		// Create the hermitian product as described in Theorem 6 of arXiv:1307.6147
		// We start by creating a list 'hermprod' containing the original tableau
		// and a second list 'is_decomposed' which contains a flag for whether the ith term
		// in hermprod has been decomposed. We then iterate through all the elements of hermprod
		// applying Y_n -> P_{n-1} Y_n P_{n-1} until no P's are left in the list.
		std::vector<tab_t> hermprod(1, tab);
		std::vector<bool> is_decomposed(1, is_trivial(tab));
		for (int i = 0; i < (int)hermprod.size(); ++i) {
			if (!is_decomposed[i]) {
				// Sandwich hermprod[i] between reduced
				is_decomposed[i] = true;
				auto reduced = reduce_tab(hermprod[i]);
				auto triv = is_trivial(reduced);
				hermprod.insert(hermprod.begin() + i + 1, reduced);
				is_decomposed.insert(is_decomposed.begin() + i + 1, triv);
				hermprod.insert(hermprod.begin() + i, reduced);
				is_decomposed.insert(is_decomposed.begin() + i, triv);
				--i;
			}
		}

		// Collect the symmetrizers. We begin with an object which has
		// independent=true and indices contains one element, which is the normalisation
		// of the overall product. We don't actually fill in the normalisation now, as we will first
		// divide out by the GCD so we wont run the risk of overflowing int
		symmetrizers.emplace_back(false, true);
		mpz_class norm = 1;
		for (const auto& herm : hermprod) {
			norm *= herm.hook_length_prod();
			for (size_t col = 0; col < herm.row_size(0); ++col) {
				if (herm.column_size(col) > 1) {
					symmetrizer_t sym(true, false);
					sym.indices.assign(herm.begin_column(col), herm.end_column(col));
					std::sort(sym.indices.begin(), sym.indices.end());
					symmetrizers.push_back(std::move(sym));
				}
			}
			for (size_t row = 0; row < herm.number_of_rows(); ++row) {
				if (herm.row_size(row) > 1) {
					symmetrizer_t sym(false, false);
					sym.indices.assign(herm.begin_row(row), herm.end_row(row));
					std::sort(sym.indices.begin(), sym.indices.end());
					symmetrizers.push_back(std::move(sym));
				}
			}
		}
		norms.push_back(norm);

	}

	// Get the GCD of the norms
	mpz_class gcd = 1;
	if (norms.size() > 1) {
		mpz_gcd(gcd.get_mpz_t(), norms[0].get_mpz_t(), norms[1].get_mpz_t());
		for (size_t i = 2; i < norms.size(); ++i)
			mpz_gcd(gcd.get_mpz_t(), gcd.get_mpz_t(), norms[i].get_mpz_t());
	}

	size_t pos = 0;
	for (auto& symmetrizer : symmetrizers) {
		if (symmetrizer.independent) {
			mpz_class norm = norms[pos] / gcd;
			symmetrizer.indices.push_back(norm.get_si());
			++pos;
		}
	}

	return false;
}

void meld::symmetrize(ProjectedTerm& projterm, const std::vector<symmetrizer_t>& symmetrizers)
{
	if (project_as_sum)
		return symmetrize_as_sum(projterm, symmetrizers);
	else
		return symmetrize_as_product(projterm, symmetrizers);
}

void meld::symmetrize_as_product(ProjectedTerm& projterm, const std::vector<symmetrizer_t>& symmetrizers)
{
	Adjform seed = projterm.ident;
	int seed_value = 1;

	if (seed.empty())
		throw std::runtime_error("symmetrize_as_product received term with no indices");

	// Keep track of which symmetrizers we have applied. Note: do not mistake this for us applying
	// the symmetrizers out-of-order: we will first apply the 'independent' symmetrizers which
	// commute with every other symmetrizer so really this is an alternative to reordering the
	// elements in the 'symmetrizers' vector by commuting elements through each other
	std::vector<bool> applied(symmetrizers.size(), false);

	// Calculate the independent symmetrizers (those which have no overlap with any other symmetrizer).
	// This means that it needs the 'independent' flag AND no dummy indices. Then use these to sort the
	// independent indices in seed and possibly pick up a factor of -1.
	for (size_t i = 0; i < symmetrizers.size(); ++i) {
		bool independent =
			symmetrizers[i].independent &&
			std::all_of(symmetrizers[i].indices.begin(), symmetrizers[i].indices.end(),
				[seed](size_t i) { return seed[i] < 0; });
		if (independent) {
			Adjform indices;
			for (const auto& index : symmetrizers[i].indices)
				indices.push_coordinate(seed[index]); // push_coordinate is more efficient if we know there are no dummy indices
			std::vector<Adjform::value_type> sorted_indices(indices.begin(), indices.end());
			std::sort(sorted_indices.begin(), sorted_indices.end());
			for (size_t j = 0; j < (size_t)indices.size(); ++j) {
				auto idx1 = indices[j];
				auto idx2 = sorted_indices[j];
				if (idx1 != idx2) {
					if (symmetrizers[i].antisymmetric)
						seed_value *= -1;
					auto pos1 = seed.index_of(idx1);
					auto pos2 = seed.index_of(idx2);
					seed.swap(pos1, pos2);
					indices.swap(j, indices.index_of(idx2));
				}
			}
			applied[i] = true;
		}
	}

	// Shared-dummy optimization: see if the symmetrizer at the front has cancellations (a la
	// logic in symmetrize_as_product) taking into account dummy positions. We can only do this
	// with the front of the symmetriers as after this the dummies will be mixed up. We rewrite
	// the symmetrizers replacing index positions with their dummy equivalents if this points to
	// a lower slot and then look for cancellations.

	// Find first two unapplied terms
	auto first_not_applied = std::find(applied.begin(), applied.end(), false);
	auto second_not_applied = first_not_applied == applied.end()
		? applied.end()
		: std::find(first_not_applied + 1, applied.end(), false);

	if (second_not_applied != applied.end()) {
		auto remove_dummies = [seed](Adjform::value_type idx) { return (seed[idx] < idx && seed[idx] >= 0) ? (size_t)seed[idx] : idx; };

		// Remove dummies from first term
		size_t first_idx = std::distance(applied.begin(), first_not_applied);
		const auto& first = symmetrizers[first_idx];
		std::vector<size_t> first_nd(first.indices.size());
		std::transform(first.indices.begin(), first.indices.end(), first_nd.begin(), remove_dummies);
		std::sort(first_nd.begin(), first_nd.end());

		// Remove dummies from second term
		size_t second_idx = std::distance(applied.begin(), second_not_applied);
		const auto& second = symmetrizers[second_idx];
		std::vector<size_t>second_nd(second.indices.size());
		std::transform(second.indices.begin(), second.indices.end(), second_nd.begin(), remove_dummies);
		std::sort(second_nd.begin(), second_nd.end());

		// Get intersection and union
		std::vector<size_t> uni, inter;
		std::set_union(first_nd.begin(), first_nd.end(), second_nd.begin(), second_nd.end(), std::back_inserter(uni));
		std::set_intersection(first_nd.begin(), first_nd.end(), second_nd.begin(), second_nd.end(), std::back_inserter(inter));
		if (first.antisymmetric == second.antisymmetric) {
			// Both symmetric/antisymmetric: can be combined if one is a subset of the other.
			if (first_nd == uni || second_nd == uni) {
				if (first_nd.size() < second_nd.size())
					*first_not_applied = true;
				else
					*second_not_applied = true;
			}
		}
		else {
			// One is symmetric and the other antisymmetric: if they overlap by more than one index
			// then the whole projection is identically zero
			if (inter.size() > 1) {
				return;
			}
		}
	}

	// Seed the symmetrized expression
	projterm.projection.add(seed, seed_value);

	// Go over the rest of the symmetrizers and apply them as normal
	for (size_t i = 0; i < symmetrizers.size(); ++i) {
		if (!applied[i]) {
			projterm.projection.apply_young_symmetry(symmetrizers[i].indices, symmetrizers[i].antisymmetric);
		}
	}

	// Symmetrize in identical tensors and we're done!
	symmetrize_idents(projterm);
}

void meld::symmetrize_as_sum(ProjectedTerm& projterm, const std::vector<symmetrizer_t>& symmetrizers)
{
	ProjectedAdjform cur;
	Adjform seed = projterm.ident;

	// Get the product of all normalizations
	ProjectedAdjform::integer_type overall_norm = 1;
	for (size_t i = 0; i < symmetrizers.size(); ++i) {
		if (symmetrizers[i].independent)
			overall_norm *= symmetrizers[i].indices[0];
	}

	for (size_t i = 0; i < symmetrizers.size(); ++i) {
		if (symmetrizers[i].independent) {
			// The independent flag here tells us that this just contains the normalisation
			// for the following product of symmetrizers. To keep things integer, we multiply by
			// the overall normalisation and then divide by the normalization for this group
			projterm.projection += cur;
			cur.clear();
			cur.set(seed, overall_norm / symmetrizers[i].indices[0]);
		}
		else {
			cur.apply_young_symmetry(symmetrizers[i].indices, symmetrizers[i].antisymmetric);
		}
	}
	projterm.projection += cur;

	symmetrize_idents(projterm);
}

// Store information about how to symmetrize in identical tensors
struct Ident {
	Ident() : n_indices(0) {}
	size_t n_indices;
	std::vector<Ex::iterator> its;
	std::vector<size_t> positions;

	std::vector<std::vector<int>> generate_commutation_matrix(const Kernel& kernel) const
	{
		Ex_comparator comp(kernel.properties);
		std::vector<std::vector<int>> cm(its.size(), std::vector<int>(its.size()));
		for (size_t i = 0; i < its.size(); ++i) {
			for (size_t j = 0; j < its.size(); ++j) {
				if (i == j)
					continue;
				cm[i][j] = comp.can_move_adjacent(Ex::parent(its[i]), its[i], its[j]) * comp.can_swap(its[i], its[j], Ex_comparator::match_t::subtree_match);
			}
		}
		return cm;
	}
};

void meld::symmetrize_idents(ProjectedTerm& projterm)
{
	// Symmetrize in identical tensors
	auto prod = projterm.tensor.begin();
	if (*prod->name != "\\prod")
		return;

	// Map holding hash of tensor -> { number of indices, {pos1, pos2, ...} }
	std::map<nset_t::iterator, Ident, nset_it_less> idents;
	size_t pos = 0;
	for (Ex::sibling_iterator beg = prod.begin(), end = prod.end(); beg != end; ++beg) {
		auto elem = idents.insert({ beg->name, {} });
		auto& ident = elem.first->second;
		if (elem.second) {
			// Insertion took place, count indices
			iter_indices indices(kernel.properties, beg);
			ident.n_indices = indices.size();
		}
		ident.its.push_back(beg);
		ident.positions.push_back(pos);
		pos += ident.n_indices;
	}
	for (const auto& ident : idents) {
		if (ident.second.positions.size() != 1) {
			projterm.projection.apply_ident_symmetry(
				ident.second.positions, ident.second.n_indices,
				ident.second.generate_commutation_matrix(kernel));
		}
	}
}


// Trace routines

bool meld::can_apply_cycle_traces(iterator it)
{
	auto trace = kernel.properties.get<Trace>(it);
	return trace && *it.begin()->name == "\\sum";
}

struct CycledTerm
{
	CycledTerm(Ex::iterator it, IndexMap& index_map, const Kernel& kernel)
		: commuting("\\sum")
		, noncommuting("\\prod")
		, it(it)
		, n_terms(0)
		, changed(false)
	{

		if (*it->name != "\\prod") {
			// A single term has nothing to commute with, so commutes by default
			auto term = commuting.append_child(commuting.begin(), it);
		}
		else {
			// The 'commuting' ex is a sum node, the first child of which is a product node representing
			// the commuting terms of 'it' (including the numeric prefactor of it).
			// If we compare against other CycledTerms and find a match, then
			// we merge the two sum nodes of the commuting term together and set the changed flag to true.
			auto commuting_head = commuting.append_child(commuting.begin(), str_node("\\prod"));
			multiply(commuting_head->multiplier, *it->multiplier);

			// Iterate through all terms in the product to see the they are commuting or noncommuting
			for (Ex::sibling_iterator beg = it.begin(), end = it.end(); beg != end; ++beg) {
				auto nc = kernel.properties.get<NonCommuting>(beg);
				auto snc = kernel.properties.get<SelfNonCommuting>(beg);
				if (nc || snc) {
					// Non-commuting term: append it to the noncommuting Ex, increment the total number of
					// terms counter and then loop through its indices appending them to the Adjform we hold
					// We also count the number of indices each term has and add this information to the
					// 'index_groups' member so that cycle() knows how many times to cycle the Adjform
					auto term = noncommuting.append_child(noncommuting.begin(), (Ex::iterator)beg);
					++n_terms;
					size_t n_indices = 0;
					for (auto& index : iter_indices(kernel.properties, term)) {
						indices.push(index, index_map, kernel);
						++n_indices;
					}
					index_groups.push_back(n_indices);
				}
				else {
					auto term = commuting.append_child(commuting_head, (Ex::iterator)beg);
				}
			}
			cleanup_dispatch(kernel, commuting, commuting_head);
		}
	}

	void cycle(const Kernel& kernel)
	{
		// Rotate noncommuting
		Ex::iterator head = noncommuting.begin();
		Ex::sibling_iterator first = head.begin(), last = head.end();
		--last;
		noncommuting.move_before(first, last);
		// Rotate indices
		if (index_groups.size() > 1) {
			indices.rotate(index_groups.back());
			std::rotate(index_groups.begin(), index_groups.end() - 1, index_groups.end());
		}
	}

	bool compare(const Kernel& kernel, const CycledTerm& other)
	{
		if (indices != other.indices)
			return false;

		Ex_comparator comp(kernel.properties);
		auto res = comp.equal_subtree(noncommuting.begin(), other.noncommuting.begin());
		return res == Ex_comparator::match_t::subtree_match ||
				 res == Ex_comparator::match_t::match_index_less ||
				 res == Ex_comparator::match_t::match_index_greater;
	}

	Ex commuting, noncommuting; // Commuting and non-commuting parts of the expression
	Adjform indices; // Index structure of the groups
	std::vector<size_t> index_groups; // Number of indices in each 'noncommuting' term
	Ex::iterator it; // The iterator this object is constructed from
	size_t n_terms; // Number of non-commuting terms
	bool changed; // Flag to be set if the commuting part of this object is modified but 'it' is not updated
};

bool meld::apply_cycle_traces(iterator it)
{
	assert(*it.begin()->name == "\\sum");
	bool applied = false;
	std::vector<CycledTerm> terms;
	for (const auto& term : split_it(it.begin(), "\\sum"))
		terms.emplace_back(term, index_map, kernel);
	for (size_t i = 0; i < terms.size(); ++i) {
		for (size_t j = i + 1; j < terms.size(); ++j) {
			if (terms[i].n_terms != terms[j].n_terms)
				continue;
			for (size_t k = 0; k <= terms[j].n_terms; ++k) {
				if (terms[i].compare(kernel, terms[j])) {
					Ex::iterator head = terms[j].commuting.begin();
					for (Ex::sibling_iterator beg = head.begin(), end = head.end(); beg != end; ++beg)
						terms[i].commuting.append_child(terms[i].commuting.begin(), (Ex::iterator)beg);
					node_zero(terms[j].it);
					applied = true;
					terms[i].changed = true;
					terms.erase(terms.begin() + j);
					--j;
					break;
				}
				terms[j].cycle(kernel);
			}
		}
	}
	for (const auto& term : terms) {
		if (term.changed) {
			tr.erase_children(term.it);
			it = tr.replace(term.it, str_node("\\prod"));
			tr.append_child(it, term.commuting.begin());
			tr.append_child(it, term.noncommuting.begin());
			cleanup_dispatch(kernel, tr, it);
		}
	}
	return applied;
}

//bool meld::can_apply_side_relations(iterator it)
//{
//	return *it->name == "\\sum";
//}
//
//
//std::vector<Ex> collect_bases(Ex::iterator it)
//{
//	assert(*it->name == "\\equals");
//	std::vector<Ex> terms;
//
//	// Get terms on left hand side
//	Ex::sibling_iterator side = it.begin();
//	for (const auto& term : split_sum(side)) {
//		terms.push_back(term);
//	}
//
//	// Get terms on right hand side
//	++side;
//	for (const auto& term : split_sum(side)) {
//		terms.push_back(term);
//		multiply(terms.back().begin()->multiplier, -1);
//	}
//
//	std::vector<Ex> bases;
//	for (size_t i = 0; i < terms.size(); ++i) {
//		Ex basis("\\equals");
//		basis.append_child(basis.begin(), terms[i].begin());
//		Ex sum("\\sum");
//		multiply(sum.begin()->multiplier, mpq_class(1, 2));
//		for (size_t j = 0; j < terms.size(); ++j)
//			sum.append_child(sum.begin(), terms[j].begin());
//		basis.append_child(basis.begin(), sum.begin());
//		bases.push_back(basis);
//	}
//
//	return bases;
//}
//
//bool meld::apply_side_relations(iterator it)
//{
//	return false;
//	assert(*it->name == "\\sum");
//
//	std::vector<Ex> bases;
//	if (*side_relations.begin()->name == "\\comma") {
//		auto top = side_relations.begin();
//		for (Ex::sibling_iterator beg = top.begin(), end = top.end(); beg != end; ++beg) {
//			auto subbases = collect_bases(beg);
//			bases.insert(bases.end(), subbases.begin(), subbases.end());
//		}
//	}
//	else if (*side_relations.begin()->name == "\\equals") {
//		bases = collect_bases(side_relations.begin());
//	}
//	else {
//		throw std::runtime_error("meld: side_relations is not a relation or comma separated list of relations");
//	}
//
//	// Iterate through all terms in 'it' calculating their projections in terms of side relations
//	std::vector<std::tuple<Ex::iterator>> projected_terms;
//	for (const auto& term : split_sum(it)) {
//		// Loop through the bases to find a match
//		for (const auto& basis : bases) {
//			auto lhs = basis.begin().begin(); 
//			if (*lhs->name == "\\prod") {
//				// If term is not a product then it can't match
//				if (*term->name != "\\prod")
//					continue;
//				// If it is a product, then iterate through the terms hoping to find a range of
//				// terms which matches 'basis'
//				auto curterm = lhs.begin();
//				Ex::iterator matchpos = term.end();
//				for (Ex::sibling_iterator beg = term.begin(), end = term.end(); beg != end; ++beg) {
//					if (similar_form(curterm, beg)) {
//						++curterm;
//						if (curterm == lhs.end()) {
//							auto next = beg;
//							++next;
//							if (next == end) {
//								matchpos = beg;
//								break;
//							}
//						}
//					}
//					else {
//						curterm = lhs.begin();
//					}
//				}
//				if (matchpos != term.end()) {
//					// Found a matching basis term
//					Ex prefactor("\\prod"), base("\\prod");
//					multiply(prefactor.begin()->multiplier, *term->multiplier);
//					Ex* on = &prefactor;
//					for (Ex::sibling_iterator beg = term.begin(), end = term.end(); beg != end; ++beg) {
//						if (curterm == beg)
//							on = &base;
//						on->append_child(beg);
//					}
//					std::map<std::pair<Ex::iterator, Adjform>, mpq_class> projection;
//					++lhs;
//					for (const auto& projterm : split_sum(lhs))
//						projection[{projterm, Adjform(projterm, index_map, kernel)}] = 1;
//				}
//			}
//		}
//	}
// }
