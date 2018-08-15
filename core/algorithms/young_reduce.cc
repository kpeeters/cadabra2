#include <map>

#include "Compare.hh"
#include "Cleanup.hh"

#include "algorithms/young_reduce.hh"
#include "algorithms/young_project_tensor.hh"
#include "algorithms/sort_sum.hh"
#include "algorithms/collect_terms.hh"
#include "algorithms/distribute.hh"

#include <sstream>
#include "DisplayTerminal.hh"

using namespace cadabra;

//std::string to_string(const Ex& ex, const Kernel& k)
//{
//	std::stringstream ss;
//	DisplayTerminal dt(k, ex, false);
//	dt.output(ss);
//	return ss.str();
//}

// Precalculates the iterators for all children of a node
struct get_children_iterators
{
	using iterator = Ex::iterator;
	using sibling_iterator = Ex::sibling_iterator;

	get_children_iterators(Ex& ex, Ex::iterator it)
	{
		for (sibling_iterator sib = ex.child(it, 0); ex.is_valid(sib); ++sib)
			its.push_back(sib);
	}

	std::vector<iterator>::iterator begin()
	{
		return its.begin();
	}
	std::vector<iterator>::iterator end()
	{
		return its.end();
	}

private:
	std::vector<iterator> its;
};

bool can_recursive_project(const Kernel& kernel, Ex& ex, Ex::iterator it)
{
	if (*it->name == "\\sum" || *it->name == "\\prod") {
		for (auto child : get_children_iterators(ex, it)) {
			if (!can_recursive_project(kernel, ex, child)) return false;
		}
		return true;
	}
	else {
		young_project_tensor ypt(kernel, ex, false);
		return ypt.can_apply(it);
	}

}

void recursive_project(const Kernel& kernel, Ex& ex, Ex::iterator it)
{
	if (*it->name == "\\sum" || *it->name == "\\prod") {
		std::vector<Ex> children;
		for (auto child : get_children_iterators(ex, it)) {
			Ex tmp(child);
			recursive_project(kernel, tmp, tmp.begin());
			children.push_back(tmp);
		}
		ex.erase_children(it);
		for (const auto& child : children)
			ex.append_child(it, child.begin());
	}
	else {
		young_project_tensor ypt(kernel, ex, false);
		if (ypt.can_apply(it)) ypt.apply(it);
	}

	cleanup_dispatch(kernel, ex, it);
	distribute dist(kernel, ex);
	if (dist.can_apply(ex.begin())) dist.apply(ex.begin());
	collect_terms ct(kernel, ex);
	if (ct.can_apply(ex.begin())) ct.apply(ex.begin());
	sort_sum ss(kernel, ex);
	if (ss.can_apply(ex.begin())) ss.apply(ex.begin());
}


multiplier_t linear_divide(const Kernel& kernel, const Ex& num, const Ex& den)
{
	if (num.begin().number_of_children() != den.begin().number_of_children()) {
		return 0;
	}

	multiplier_t factor = *(num.child(num.begin(), 0)->multiplier) / *(den.child(den.begin(), 0)->multiplier);
	for (auto it = num.child(num.begin(), 0), jt = den.child(den.begin(), 0); num.is_valid(it) && den.is_valid(jt); ++it, ++jt) {
		Ex ex1(it);
		Ex ex2(jt);
		one(ex1.begin()->multiplier);
		one(ex2.begin()->multiplier);
		Ex_comparator comp(kernel.properties);
		auto res = comp.equal_subtree(ex1.begin(), ex2.begin(), Ex_comparator::useprops_t::never);
		if (res == Ex_comparator::match_t::subtree_match) {
			multiplier_t result = *it->multiplier / *jt->multiplier;
			if (factor != result) {
				return 0;
			}
		}
		else {
			return 0;
		}
	}
	return factor;
}

std::vector<Ex> all_index_permutations(Ex ex, const Kernel& kernel)
{
	std::vector<Ex> permutations;
	if (*ex.begin()->name == "\\prod") {
		std::vector<std::vector<Ex>> child_permutations;
		std::vector<std::vector<Ex>::iterator> its;
		for (auto child : get_children_iterators(ex, ex.begin())) {
			child_permutations.push_back(all_index_permutations(Ex(child), kernel));
			its.push_back(child_permutations.back().begin());
		}

		if (its.empty())
			return std::vector<Ex>();

		while (its[0] != child_permutations[0].end()) {
			Ex cur("\\prod");
			for (auto it : its)
				cur.append_child(cur.begin(), it->begin());
			permutations.push_back(cur);

			++its.back();
			for (int i = its.size() - 1; (i > 0) && its[i] == child_permutations[i].end(); --i) {
				its[i] = child_permutations[i].begin();
				++its[i - 1];
			}
		}
	}
	else {
		std::vector<Ex> indices;

		for (auto child : get_children_iterators(ex, ex.begin()))
			indices.push_back(Ex(child));

		ex.erase_children(ex.begin());

		while (std::next_permutation(indices.begin(), indices.end(), [](const Ex& lhs, const Ex& rhs) {return *lhs.begin()->name < *rhs.begin()->name; }));
		
		do {
			Ex cur = ex;
			for (auto index : indices)
				cur.append_child(cur.begin(), index.begin());
			permutations.push_back(cur);
		} while (std::next_permutation(indices.begin(), indices.end(), [](const Ex& lhs, const Ex& rhs) {return *lhs.begin()->name < *rhs.begin()->name; }));
	}

	return permutations;
}

bool is_index_permutation(const Kernel& kernel, Ex::iterator lhs, Ex::iterator rhs)
{
	if (*lhs->name == "\\prod") {
		if (*rhs->name != "\\prod") return false;
		if (lhs.number_of_children() != rhs.number_of_children()) return false;
		Ex elhs(lhs), erhs(rhs);
		for (Ex::sibling_iterator lit = elhs.child(elhs.begin(), 0), rit = erhs.child(erhs.begin(), 0); elhs.is_valid(lit); ++lit, ++rit) {
			if (!is_index_permutation(kernel, lit, rit)) return false;
		}
		return true;
	}
	else {
		Ex_comparator comp(kernel.properties);
		auto ret = comp.equal_subtree(lhs, rhs, Ex_comparator::useprops_t::never);
		return 
			ret == Ex_comparator::match_t::match_index_greater ||
			ret == Ex_comparator::match_t::subtree_match;
	}
}

multiplier_t check_equivalence(const Kernel& kernel, Ex lhs, Ex rhs)
{
	recursive_project(kernel, lhs, lhs.begin());
	recursive_project(kernel, rhs, rhs.begin());
	return linear_divide(kernel, lhs, rhs);
}

bool is_zero(const Kernel& kernel, Ex ex)
{
	recursive_project(kernel, ex, ex.begin());
	return ex == 0;
}

young_reduce::young_reduce(const Kernel& kernel, Ex& ex)
	: Algorithm(kernel, ex)
{

}

bool young_reduce::can_apply(iterator it)
{
	return *it->name == "\\sum";
}

young_reduce::result_t young_reduce::apply(iterator& it)
{
	result_t ret = result_t::l_no_action;

	std::vector<std::vector<Ex::iterator>> sets;

	for (auto child : get_children_iterators(tr, it)) {
		if (can_recursive_project(kernel, tr, child)) {
			bool found_set = false;
			for (auto& collection : sets) {
				if (is_index_permutation(kernel, child, collection[0])) {
					collection.push_back(child);
					found_set = true;
					break;
				}
			}
			if (!found_set)
				sets.push_back(std::vector<Ex::iterator>(1, child));
		}
	}

	int i = 0;
	for (const auto& set : sets) {
		++i;
		bool replacement_made = false;
		for (auto permutation : all_index_permutations(set[0], kernel)) {
			if (replacement_made)
				break;

			for (int n_terms = set.size(); n_terms > 1; --n_terms) {
				if (replacement_made)
					break;

				std::vector<bool> v(set.size(), false);
				std::fill(v.begin(), v.begin() + n_terms, true);

				do {
					Ex combination("\\sum");
					for (size_t k = 0; k < v.size(); ++k) {
						if (v[k]) 
							combination.append_child(combination.begin(), set[k]);
					}
					if (is_zero(kernel, combination)) {
						replacement_made = true;
						for (size_t k = 0; k < v.size(); ++k) {
							if (v[k])
								tr.erase(set[k]);
						}
						tr.append_child(it, str_node("0"));
					}
					else {
						multiplier_t factor = check_equivalence(kernel, combination, permutation);
						if (factor != 0) {
							replacement_made = true;
							for (size_t k = 0; k < v.size(); ++k) {
								if (v[k])
									tr.erase(set[k]);
							}
							multiply(permutation.begin()->multiplier, factor);
							tr.append_child(it, permutation.begin());
							ret = result_t::l_applied;

						}
					}
				} while (std::prev_permutation(v.begin(), v.end()));
			}
		}
	}

	return ret;
}
