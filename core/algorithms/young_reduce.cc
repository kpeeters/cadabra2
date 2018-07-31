#include <map>
#include <sstream>
#include "DisplayTerminal.hh"
#include "Compare.hh"
#include "Cleanup.hh"

#include "algorithms/young_reduce.hh"
#include "algorithms/young_project_tensor.hh"
#include "algorithms/sort_sum.hh"
#include "algorithms/collect_terms.hh"

using namespace cadabra;

std::string to_string(const Ex& ex, const Kernel& k)
{
	std::stringstream ss;
	DisplayTerminal dt(k, ex, false);
	dt.output(ss);
	return ss.str();
}

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

multiplier_t linear_divide(const Kernel& kernel, const Ex& num, const Ex& den)
{
	if (num.begin().number_of_children() != den.begin().number_of_children())
		return 0;

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
	}
	return factor;
}

std::map<Ex, std::vector<Ex::iterator>> all_combinations(const Kernel& kernel, std::map<Ex::iterator, Ex> map, Ex::iterator exclude)
{
	std::map<Ex, std::vector<Ex::iterator>> ret;
	map.erase(exclude);

	for (int n_terms = 1; n_terms <= map.size(); ++n_terms) {
		// Initialize vector containing a bitmask of the terms we are going
		// to combine
		std::vector<bool> v(map.size(), false);
		std::fill(v.begin(), v.begin() + n_terms, true);

		// Loop over all arrangements of bitmaskings
		do {
			Ex ex("\\sum");
			std::vector<Ex::iterator> its;
			// Loop over the bitmask, appending that combination of terms to the output
			for (int k = 0; k < v.size(); ++k) {
				if (v[k]) {
					ex.append_child(ex.begin(), std::next(map.begin(), k)->second.begin());
					its.push_back(std::next(map.begin(), k)->first);
				}
			}
			// 'Canonicalise'ish
			cleanup_dispatch(kernel, ex, ex.begin());
			collect_terms ct(kernel, ex);
			if (ct.can_apply(ex.begin())) ct.apply(ex.begin());
			sort_sum ss(kernel, ex);
			if (ss.can_apply(ex.begin())) ss.apply(ex.begin());
			ret[ex] = its;
		} while (std::prev_permutation(v.begin(), v.end()));
	}
	return ret;
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
	while (can_apply(it)) {
		std::map<iterator, Ex> projections;
		for (auto child : get_children_iterators(tr, it)) {
			Ex ex(child);
			bool is_projected = false;
			young_project_tensor ypt(kernel, ex, false);
			if (ypt.can_apply(ex.begin())) {
				ypt.apply(ex.begin());
				is_projected = true;
			}
			else if (*ex.begin()->name == "\\prod") {
				// Product of terms, project all child nodes
				for (iterator i : get_children_iterators(ex, ex.begin())) {
					if (ypt.can_apply(i)) {
						ypt.apply(i);
						is_projected |= true;
					}
				}
			}
			if (is_projected) {
				sort_sum ss(kernel, ex);
				if (ss.can_apply(ex.begin())) ss.apply(ex.begin());
				projections[child] = ex;
			}
		}

		bool is_modified = false;
		std::vector<iterator> invalid_nodes;
		for (const auto& pair : projections) {
			if (is_modified)
				break;
			if (std::find(invalid_nodes.begin(), invalid_nodes.end(), pair.first) != invalid_nodes.end())
				continue;
			for (const auto& combination : all_combinations(kernel, projections, pair.first)) {
				multiplier_t fact = linear_divide(kernel, combination.first, pair.second);
				if (fact != 0) {
					std::cerr << to_string(combination.first, kernel) << " is " << fact << " * " << to_string(pair.second, kernel) << '\n';
					std::cerr << "Multiplying " << to_string(pair.first, kernel) << " by " << (1 + fact) << '\n';
					multiply(pair.first->multiplier, 1 + fact);
					for (auto old : combination.second) {
						tr.erase(old);
						invalid_nodes.push_back(old);
					}
					cleanup_dispatch(kernel, tr, it);
					is_modified = true;
					break;
				}
			}
		}
		std::cerr << "Pass finished, we now have " << to_string(Ex(it), kernel);
		if (!is_modified)
			break;
	}

	return result_t::l_applied;
}
