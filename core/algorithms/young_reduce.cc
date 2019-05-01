#include <iostream>
#include <iomanip>
#include <map>
#include <vector>
#include <algorithm>
#include <numeric>
#include <regex>
#include <sstream>

#include "Compare.hh"
#include "Cleanup.hh"
#include "DisplayTerminal.hh"
#include "algorithms/young_reduce.hh"
#include "properties/TableauSymmetry.hh"

using namespace cadabra;

using index_t = young_reduce::index_t;
using indices_t = young_reduce::indices_t;
using tableau_t = TableauSymmetry::tab_t;
using terms_t = young_reduce::terms_t;

template <typename ValueT>
std::map<ValueT, std::vector<size_t>> invert_vector(const std::vector<ValueT>& vec)
{
	std::map<ValueT, std::vector<size_t>> ret;
	for (size_t i = 0; i < vec.size(); ++i)
		ret[vec[i]].push_back(i);
	return ret;
}


//----------------------------------------
class Symmetry
{
public:
	Symmetry(bool antisymmetric)
		: antisymmetric{ antisymmetric }
	{
		reset();
	}

	Symmetry(const indices_t& indices, bool antisymmetric)
		: antisymmetric{ antisymmetric }
	{
		set_indices(indices);
		reset();
	}

	static std::vector<Symmetry> Symmetry::enumerate_tableau(tableau_t& tab)
	{
		std::vector<Symmetry> syms;

		for (size_t row = 0; row < tab.number_of_rows(); ++row)
			syms.emplace_back(indices_t(tab.begin_row(row), tab.end_row(row)), false);

		for (size_t col = 0; col < tab.row_size(0); ++col)
			syms.emplace_back(indices_t(tab.begin_column(col), tab.end_column(col)), true);

		return syms;
	}


	void set_indices(const indices_t& new_indices)
	{
		indices = new_indices;
		std::sort(indices.begin(), indices.end());
		reset();
	}

	void add_offset(int n)
	{
		for (size_t i = 0; i < indices.size(); ++i) {
			indices[i] += n;
			perm[i] += n;
		}
	}

	size_t n_perms() const
	{
		size_t ret = 1;
		for (size_t i = 1; i <= indices.size(); ++i)
			ret *= i;
		return ret;
	}

	bool next()
	{
		if (antisymmetric && (flip = !flip))
			parity *= -1;
		return std::next_permutation(indices.begin(), indices.end());
	}

	void reset()
	{
		perm = indices;
		flip = false;
		parity = 1;
	}

	std::pair<indices_t, int> apply(const indices_t & original) const
	{
		auto ret = std::make_pair(original, parity);
		for (int i = 0; i < indices.size(); ++i) {
			ret.first[indices[i]] = original[perm[i]];
		}
		for (auto index : indices) {
			if (ret.first[index] >= 0)
				ret.first[ret.first[index]] = index;
		}
		return ret;
	}

	indices_t indices, perm;
	bool antisymmetric : 1;
	bool flip : 1;
	int parity : 2;
};


//----------------------------------------
void swap_indices(indices_t& in, index_t beg_1, index_t beg_2, index_t n)
{
	for (index_t i = 0; i < n; ++i) {
		// 
		if (in[beg_1] != beg_2 && in[beg_2] != beg_1) {
			std::swap(in[beg_1 + i], in[beg_2 + i]);
			if (in[beg_1 + i] >= 0)
				in[in[beg_1 + i]] = beg_1 + i;
			if (in[beg_2 + i] >= 0)
				in[in[beg_2 + i]] = beg_2 + i;
		}
	}
}

//----------------------------------------
mpq_class linear_compare(const terms_t& a, const terms_t& b)
{
	if (a.empty() || a.size() != b.size())
		return 0;

	auto a_it = a.begin(), b_it = b.begin(), a_end = a.end();
	mpq_class factor = a_it->second / b_it->second;
	while (a_it != a_end) {
		if (a_it->second / b_it->second != factor)
			return 0;
		++a_it, ++b_it;
	}
	return factor;
}


//----------------------------------------
young_reduce::young_reduce(const Kernel& kernel, Ex& ex, const Ex& pattern)
	: Algorithm(kernel, ex)
	, pat(pattern)
{
	std::cerr << "Constructing young_reduce object with ex:\n" << ex << "\npattern:\n" << pattern << '\n';
	pat_decomp = symmetrize(pattern.begin());
}

//----------------------------------------
terms_t young_reduce::symmetrize(Ex::iterator it)
{
	std::cerr << "Symmetrizing\n" << Ex(it) << '\n';
	std::vector<std::pair<nset_t::iterator, size_t>> names;
	indices_t indices;
	std::vector<Symmetry> symmetries;

	Ex::iterator beg, end;
	if (*it->name == "\\prod") {
		beg = it.begin();
		end = it.end();
	}
	else {
		beg = it;
		end = it;
		++end;
	}

	// Iterate over terms in product, or just `it` if no product, and populate
	// `names`, `indices` and `symmetries`
	while (beg != end) {
		// Populate `symmetries`
		auto tb = kernel.properties.get_composite<TableauBase>(beg);
		if (tb) {
			auto tab = tb->get_tab(kernel.properties, tr, beg, 0);
			auto syms = Symmetry::enumerate_tableau(tab);
			for (auto& sym : syms)
				sym.add_offset(indices.size());
			symmetries.insert(symmetries.end(), syms.begin(), syms.end());
		}

		size_t n_indices = 0;
		//Iterate over children to populate `indices`
		for (auto idx = beg.begin(), edx = beg.end(); idx != edx; ++idx) {
			if (idx->is_index()) {
				++n_indices;
				auto pos = std::find(index_map.begin(), index_map.end(), beg->name);
				if (pos == index_map.end()) {
					indices.push_back(index_map.size());
					index_map.push_back(beg->name);
				}
				else {
					indices.push_back(std::distance(index_map.begin(), pos));
				}
			}
		}
		// Populate `names`
		names.emplace_back(beg->name, n_indices);

		++beg;
	}

	indices_t identity(indices.size(), -1);
	for (size_t i = 0; i < identity.size(); ++i) {
		if (identity[i] >= 0) {
			continue;
		}
		auto pos = std::distance(indices.begin(), std::find(indices.begin() + i + 1, indices.end(), indices[i]));
		if (pos == indices.size()) {
			identity[i] = -indices[i];
		}
		else {
			identity[i] = static_cast<index_t>(pos);
			identity[pos] = static_cast<index_t>(i);
		}
	}

	terms_t terms;
	terms[identity] = 1;

	// Symmetrize in identical tensors
	index_t pos_i = 0;
	for (auto i = 0; i < names.size(); ++i) {
		index_t pos_j = pos_i + (index_t)names[i].second;
		for (auto j = i + 1; j < names.size(); ++j) {
			if (names[i] == names[j]) {
				terms_t new_terms;
				for (const auto& term : terms) {
					// Halve contribution
					new_terms[term.first] = term.second / 2;
					// Make new term
					auto new_term = term.first;
					// Apply symmetry
					swap_indices(new_term, pos_i, pos_j, (index_t)names[i].second);
					// Insert
					new_terms[new_term] = term.second / 2;
				}
				terms = new_terms;
			}
		}
		pos_i += (index_t)names[i].second;
	}

	// Young project
	for (auto symmetry : symmetries) {
		terms_t new_terms;
		for (const auto& term : terms) {
			// Divide contribution
			new_terms[term.first] += term.second / symmetry.n_perms();
			// Make new terms
			symmetry.reset();
			while (symmetry.next()) {
				auto new_term = symmetry.apply(term.first);
				new_terms[new_term.first] += new_term.second * term.second / symmetry.n_perms();
				if (new_terms[new_term.first] == 0) {
					new_terms.erase(new_term.first);
				}
			}
		}
		terms = new_terms;
	}
	return terms;
}



bool young_reduce::can_apply(iterator it)
{
	std::cerr << "Checking can_apply on \n" << Ex(it);
	// Either at a node which looks like pat, or a node which
	// is a sum of terms which look like pat

	Ex_comparator comp(kernel.properties);

	if (*it->name == "\\sum") {
		auto cur = it.begin(), end = it.end();
		while (cur != end) {
			auto match = comp.equal_subtree(pat.begin(), cur);
			if (match != Ex_comparator::match_t::match_index_greater &&
				match != Ex_comparator::match_t::match_index_less) {
				std::cerr << "...no\n";
				return false;
			}
			++cur;
		}
		std::cerr << "...yes\n";
		return true;
	}
	else {
		auto match = comp.equal_subtree(pat.begin(), it);
		auto ret = match == Ex_comparator::match_t::match_index_greater ||
			match == Ex_comparator::match_t::match_index_less;
		std::cerr << "..." << (ret ? "yes" : "no") << '\n';
		return ret;
	}
}

young_reduce::result_t young_reduce::apply(iterator& it)
{
	std::cerr << "Applying on " << Ex(it);
	terms_t decomp;
	if (*it->name == "\\sum") {
		for (auto beg = it.begin(), end = it.end(); beg != end; ++beg) {
			auto d = symmetrize(beg);
			for (const auto& kv : d) {
				decomp[kv.first] += kv.second;
			}
		}
	}
	else {
		decomp = symmetrize(it);
	}

	auto fact = linear_compare(decomp, pat_decomp);
	if (fact == 0) {
		std::cerr << "No match!\n";
		return result_t::l_no_action;
	}
	else {
		std::cerr << "Matched\n" << Ex(it) << "with factor " << fact << '\n';
		it = tr.replace(it, pat.begin());
		multiply(it->multiplier, fact);
		return result_t::l_applied;
	}
}

