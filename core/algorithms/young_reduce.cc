#include "Compare.hh"
#include "Cleanup.hh"

#include "algorithms/young_reduce.hh"
#include "algorithms/young_project_tensor.hh"
#include "algorithms/sort_sum.hh"
#include "algorithms/collect_terms.hh"
#include "algorithms/distribute.hh"
#include "algorithms/rename_dummies.hh"

#include <sstream>
#include "DisplayTerminal.hh"

using namespace cadabra;



struct get_children_iterators {
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
			} else {
			return 0;
			}
		}
	return factor;
	}
//
//struct get_index_permutations
//{
//public:
//	struct iterator
//	{
//	public:
//		using value = Ex;
//		using reference = const value&;
//		using pointer = const value*;
//
//		bool operator == (const iterator& other)
//		{
//			return cur == other.cur;
//		}
//	private:
//		friend struct get_index_permutations;
//
//		iterator(get_index_permutations& parent)
//			: parent(parent)
//		{
//		}
//
//		void construct_ex()
//		{
//			size_t cur_idx = 0;
//			for (auto node : parent.nodes) {
//				for (auto it : node.first) {
//					it.
//				}
//			}
//		}
//		get_index_permutations& parent;
//	};
//
//	get_index_permutations(const Kernel& kernel, const Ex& ex)
//		: base(ex)
//	{
//		if (*ex.begin()->name == "\\prod") {
//			for (auto child : get_children_iterators(base, base.begin()))
//				add_node(child);
//		}
//		else {
//			add_node(base.begin());
//		}
//	}
//
//	iterator begin()
//	{
//		// Reset the indices vector to its first permutation
//		while (std::next_permutation(indices.begin(), indices.end()));
//
//		return iterator();
//	}
//
//private:
//	void add_node(Ex::iterator pos)
//	{
//		nodes.push_back({ pos, base.number_of_children(pos) });
//		for (auto it : get_children_iterators(base, pos)) {
//			indices.push_back(Ex(it));
//			base.erase(it);
//		}
//	}
//
//	Ex base;
//	std::vector<Ex> indices;
//	std::vector<std::pair<Ex::iterator, size_t>> nodes;
//};


struct get_combinations {
	public:
		struct iterator {
			public:
				struct Combination {
					Ex ex;
					std::vector<Ex::iterator> its;
					};

				using value = Combination;
				using reference = const value&;
				using pointer = const value*;

				bool operator == (const iterator& other)
					{
					return (n_terms == other.n_terms) && (v == other.v);
					}

				bool operator != (const iterator& other)
					{
					return !(*this == other);
					}

				iterator& operator ++ ()
					{
					// Check for end iterator
					if (n_terms == 1)
						return *this;

					if (!std::prev_permutation(v.begin(), v.end())) {
						--n_terms;
						v = std::vector<bool>(n_terms, false);
						std::fill(v.begin(), v.begin() + n_terms, true);
						}

					construct_combination();
					return *this;
					}

				iterator operator ++ (int)
					{
					iterator other = *this;
					++(*this);
					return other;
					}

				reference operator * () const
					{
					return combination;
					}

				pointer operator -> () const
					{
					return &combination;
					}

			private:
				// Construct begin iterator
				iterator(const std::vector<Ex::iterator>& its)
					: its(its)
					, n_terms(its.size())
					, v(its.size(), true)
					{
					construct_combination();
					}

				// Construct end iterator
				iterator(const std::vector<Ex::iterator>& its, bool)
					: its(its)
					, n_terms(1)
					, v(1, true)
					{

					}

				void construct_combination()
					{
					combination.ex = Ex("\\sum");

					combination.its.clear();
					for (size_t k = 0; k < v.size(); ++k) {
						if (v[k]) {
							combination.ex.append_child(combination.ex.begin(), its[k]);
							combination.its.push_back(its[k]);
							}
						}
					std::cerr << combination.ex;
					}

				friend struct get_combinations;
				const std::vector<Ex::iterator>& its;
				int n_terms;
				std::vector<bool> v;
				Combination combination;
			};

		get_combinations(const std::vector<Ex::iterator>& its)
			: its(its)
			{

			}

		iterator begin()
			{
			return iterator(its);
			}

		iterator end()
			{
			return iterator(its, false);
			}

	private:
		const std::vector<Ex::iterator>& its;
	};



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
		} else {
		std::vector<Ex> indices;

		for (auto child : get_children_iterators(ex, ex.begin()))
			indices.push_back(Ex(child));

		ex.erase_children(ex.begin());

		while (std::next_permutation(indices.begin(), indices.end(), [](const Ex& lhs, const Ex& rhs) {
		return *lhs.begin()->name < *rhs.begin()->name;
			}));

		do {
			Ex cur = ex;
			for (auto index : indices)
				cur.append_child(cur.begin(), index.begin());
			permutations.push_back(cur);
			} while (std::next_permutation(indices.begin(), indices.end(), [](const Ex& lhs, const Ex& rhs) {
		return *lhs.begin()->name < *rhs.begin()->name;
			}));
		}

	return permutations;
	}

bool is_index_permutation(const Kernel& kernel, Ex::iterator lhs, Ex::iterator rhs);

bool is_index_permutation(const Kernel& kernel, const Ex& lhs, const Ex& rhs)
	{
	if (lhs.begin()->name == rhs.begin()->name) {
		if (lhs.number_of_children(lhs.begin()) != rhs.number_of_children(rhs.begin()))
			return false;

		Ex::sibling_iterator lit = lhs.child(lhs.begin(), 0);
		Ex::sibling_iterator rit = rhs.child(rhs.begin(), 0);

		if (lit->is_index()) {
			std::vector<std::string> lnames, rnames;
			while (lhs.is_valid(lit) && rhs.is_valid(rit)) {
				lnames.push_back(*lit->name);
				rnames.push_back(*rit->name);
				++lit, ++rit;
				}
			std::sort(lnames.begin(), lnames.end());
			std::sort(rnames.begin(), rnames.end());
			return lnames == rnames;
			} else {
			while (lhs.is_valid(lit) && rhs.is_valid(rit)) {
				if (!is_index_permutation(kernel, lit, rit))
					return false;
				++lit, ++rit;
				}
			return true;
			}
		} else {
		return false;
		}
	}

bool is_index_permutation(const Kernel& kernel, Ex::iterator lhs, Ex::iterator rhs)
	{
	return is_index_permutation(kernel, Ex(lhs), Ex(rhs));
	}

Ex project(const Kernel& kernel, Ex ex)
	{
	young_project_tensor ypt(kernel, ex, false);
	ypt.apply_generic();

	distribute dist(kernel, ex);
	dist.apply_generic();

	rename_dummies rd(kernel, ex, "", "");
	rd.apply_generic();

	collect_terms ct(kernel, ex);
	ct.apply_generic();

	sort_sum ss(kernel, ex);
	ss.apply_generic();

	return ex;
	}

std::vector<Ex::iterator> find_matching_terms(const Kernel& kernel, Ex& ex, Ex::iterator& it, const Ex& pattern)
	{
	std::vector<Ex::iterator> its;

	for (auto child : get_children_iterators(ex, it)) {
		if (is_index_permutation(kernel, child, pattern.begin()))
			its.push_back(child);
		}

	return its;
	}

young_reduce::young_reduce(const Kernel& kernel, Ex& ex, const Ex& pattern, bool search_permutations)
	: Algorithm(kernel, ex)
	, search_permutations(search_permutations)
	, pattern(pattern)
	{

	}

bool young_reduce::can_apply(iterator it)
	{
	return *it->name == "\\sum";
	}

void young_reduce::cleanup(iterator& it)
	{
	cleanup_dispatch(kernel, tr, it);
	}

young_reduce::result_t young_reduce::apply(iterator& it)
	{
	auto its = find_matching_terms(kernel, tr, it, pattern);

	auto res = reduce(it, its);
	if (res == result_t::l_no_action && search_permutations)
		res = permute(it, its);

	if (res != result_t::l_no_action)
		cleanup(it);
	return res;
	}

young_reduce::result_t young_reduce::reduce(iterator& it, const std::vector<Ex::iterator>& its)
	{
	for (auto combination : get_combinations(its)) {
		Ex projected_combination = project(kernel, combination.ex);
		if (projected_combination == 0) {
			for (auto& old : combination.its)
				tr.erase(old);
			if (tr.number_of_children(it) == 0)
				tr.append_child(it, str_node("0"));
			return result_t::l_applied;
			} else {
			for (auto& cur_it : its) {
				Ex cur(cur_it);
				Ex projected_cur = project(kernel, cur);
				multiplier_t factor = linear_divide(kernel, projected_combination, projected_cur);
				if (factor != 0) {
					for (auto& old : combination.its) {
						if (old != cur_it)
							tr.erase(old);
						}
					multiply(cur_it->multiplier, factor);
					return result_t::l_applied;
					}
				}
			}
		}
	return result_t::l_no_action;
	}

young_reduce::result_t young_reduce::permute(iterator& it, const std::vector<Ex::iterator>& its)
	{
	for (const auto& combination : get_combinations(its)) {
		Ex projected_combination = project(kernel, combination.ex);
		for (const auto& permutation : all_index_permutations(its[0], kernel)) {
			Ex projected_permutation = project(kernel, permutation);
			multiplier_t factor = linear_divide(kernel, projected_combination, projected_permutation);
			if (factor != 0) {
				for (auto& old : combination.its)
					tr.erase(old);
				iterator r = tr.append_child(it, permutation.begin());
				multiply(r->multiplier, factor);
				return result_t::l_applied;
				}
			}
		}

	return result_t::l_no_action;
	}
