#include <map>

#include "Compare.hh"
#include "Cleanup.hh"

#include "algorithms/young_reduce.hh"
#include "algorithms/young_project_tensor.hh"
#include "algorithms/sort_sum.hh"
#include "algorithms/collect_terms.hh"
#include "algorithms/distribute.hh"

using namespace cadabra;

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
		else {
			return 0;
		}
	}
	return factor;
}


struct NotProjectableException : public std::exception {};

struct Projection
{
	Projection(const Kernel& kernel, Ex::iterator child)
		: it(child), ex(child)
	{
		bool is_projected = false;

		young_project_tensor ypt(kernel, ex, false);
		if (ypt.can_apply(ex.begin())) {
			ypt.apply(ex.begin());
			is_projected = true;
		}
		else if (*ex.begin()->name == "\\prod") {
			for (Ex::iterator i : get_children_iterators(ex, ex.begin())) {
				if (ypt.can_apply(i)) {
					ypt.apply(i);
					is_projected |= true;
				}
			}
			distribute d(kernel, ex);
			if (d.can_apply(ex.begin())) d.apply(ex.begin());
		}

		if (!is_projected)
			throw NotProjectableException();

		sort_sum ss(kernel, ex);
		if (ss.can_apply(ex.begin()))
			ss.apply(ex.begin());
	}

	bool operator == (const Projection& other) { return it == other.it; }

	Ex::iterator it;
	Ex ex;
};


struct Combination
{
	Combination() : ex("\\sum") {}
	
	void add_projection(const Projection& projection) 
	{ 
		ex.append_child(ex.begin(), projection.ex.begin());
		its.push_back(projection.it);
	}

	void cleanup(const Kernel& kernel)
	{
		cleanup_dispatch(kernel, ex, ex.begin());

		collect_terms ct(kernel, ex);
		if (ct.can_apply(ex.begin())) 
			ct.apply(ex.begin());

		sort_sum ss(kernel, ex);
		if (ss.can_apply(ex.begin())) 
			ss.apply(ex.begin());
	}

	Ex ex;
	std::vector<Ex::iterator> its;
};


std::vector<Combination> all_combinations(const Kernel& kernel, std::vector<Projection> projections, Projection exclude)
{
	std::vector<Combination> combinations;
	projections.erase(std::remove(projections.begin(), projections.end(), exclude), projections.end());

	for (int n_terms = 1; n_terms <= projections.size(); ++n_terms) {
		// Initialize vector containing a bitmask of the terms we are going
		// to combine
		std::vector<bool> v(projections.size(), false);
		std::fill(v.begin(), v.begin() + n_terms, true);

		// Loop over all arrangements of bitmaskings
		do {
			Combination combination;
			// Loop over the bitmask, appending that combination of terms to the output
			for (int k = 0; k < v.size(); ++k) {
				if (v[k])
					combination.add_projection(projections[k]);
			}
			combination.cleanup(kernel);
			combinations.push_back(combination);
		} while (std::prev_permutation(v.begin(), v.end()));
	}
	return combinations;
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
	bool is_modified = true;
	while (is_modified && can_apply(it)) {
		is_modified = false;

		// Create projections
		std::vector<Projection> projections;
		for (auto child : get_children_iterators(tr, it)) {
			try {
				Projection projection(kernel, child);
				projections.push_back(projection);
			}
			catch (const NotProjectableException& npe) {
				// Don't add
			}
		}
		
		for (const auto& projection : projections) {
			if (is_modified) // Break as soon as the tree has been modified, iterators are now invalid
				break;

			auto combinations = all_combinations(kernel, projections, projection);
			for (const auto& combination : combinations) {
				multiplier_t fact = linear_divide(kernel, combination.ex, projection.ex);
				if (fact != 0) {
					multiply(projection.it->multiplier, 1 + fact);
					for (auto old : combination.its)
						tr.erase(old);
					cleanup_dispatch(kernel, tr, it);
					is_modified = true;
					break;
				}
			}
		}
	}

	return result_t::l_applied;
}
