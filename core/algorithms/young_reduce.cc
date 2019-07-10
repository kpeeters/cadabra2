#include <iostream>
#include <iomanip>
#include <map>
#include <vector>
#include <algorithm>
#include <numeric>
#include <regex>
#include <sstream>
#include <deque>

#include "Compare.hh"
#include "Cleanup.hh"
#include "Hash.hh"
#include "DisplayTerminal.hh"
#include "Stopwatch.hh"
#include "algorithms/young_reduce.hh"
#include "properties/TableauSymmetry.hh"

#define DEBUG_OUTPUT 0

#define cdebug if (!DEBUG_OUTPUT) {} else std::cerr


using namespace cadabra;

// Debugging code, to be removed eventually
template <typename It>
std::string join(It begin, It end, const std::string& delim = " ")
{
	std::stringstream ss;
	It next = std::next(begin);
	while (next != end) {
		ss << *begin << delim;
		++begin, ++next;
	}
	ss << *begin;
	return ss.str();
}

template <typename Container>
std::string join(const Container& container, const std::string& delim = " ")
{
	return join(container.begin(), container.end(), delim);
}

std::string to_string(const Ex& ex, const Kernel& kernel)
{
	DisplayTerminal dt(kernel, ex, false);
	std::stringstream ss;
	dt.output(ss);
	return '$' + ss.str() + '$';
}

std::string to_string(const Ex::iterator& ex, const Kernel& kernel)
{
	return to_string(Ex(ex), kernel);
}
//------------------------------------------------------------------------


using index_t = young_reduce::index_t;
using indices_t = young_reduce::indices_t;
using tableau_t = TableauSymmetry::tab_t;
using terms_t = young_reduce::terms_t;
using names_t = young_reduce::names_t;

struct young_reduce::IndexMap
{
	index_t name_to_idx(nset_t::iterator name)
	{
		auto pos = std::find(data_.begin(), data_.end(), name);
		if (pos == data_.end()) {
			data_.push_back(name);
			return (index_t)data_.size();
		}
		else {
			return (index_t)std::distance(data_.begin(), pos) + 1;
		}
	}

	nset_t::iterator idx_to_name(index_t idx)
	{
		return data_[idx - 1];
	}

	std::vector<nset_t::iterator> data_;
};

class Symmetry
{
public:
	Symmetry(bool antisymmetric)
		: antisymmetric{ antisymmetric }
	{
		reset();
	}

	Symmetry(const indices_t& indices, bool antisymmetric, int offset = 0)
		: antisymmetric{ antisymmetric }
	{
		set_indices(indices, offset);
		reset();
	}
	
	void set_indices(const indices_t& new_indices, int offset)
	{
		indices = new_indices;
		for (size_t i = 0; i < indices.size(); ++i)
			indices[i] += offset;
		std::sort(indices.begin(), indices.end());
		reset();
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
		for (size_t i = 0; i < indices.size(); ++i) {
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

indices_t to_pi_form(const indices_t& in)
{
	indices_t out(in.size(), -1);
	for (size_t i = 0; i < in.size(); ++i) {
		if (out[i] >= 0) {
			continue;
		}
		size_t pos = std::distance(in.begin(), std::find(in.begin() + i + 1, in.end(), in[i]));
		if (pos == in.size()) {
			out[i] = -in[i];
		}
		else {
			out[i] = static_cast<index_t>(pos);
			out[pos] = static_cast<index_t>(i);
		}
	}
	return out;
}

indices_t from_pi_form(const indices_t& in)
{
	index_t counter = 1;
	indices_t out(in.size());
	for (size_t i = 0; i < in.size(); ++i) {
		if (in[i] < 0) {
			out[i] = counter;
			++counter;
		}
		else if (i < in[i]) {
			out[i] = counter;
			++counter;
		}
		else {
			out[i] = out[in[i]];
		}
	}
	return out;
}

void swap_indices(indices_t& in, index_t beg_1, index_t beg_2, index_t n)
{
	cdebug << "performing swap: ";
	for (int i = 0; i < in.size(); ++i) {
		if (i == beg_1 || i == beg_2)
			cdebug << '[';
		cdebug << in[i];
		if (i == beg_1 + n - 1 || i == beg_2 + n - 1)
			cdebug << ']';
	}

	for (index_t i = 0; i < n; ++i) {
		if (in[beg_1 + i] == beg_2 + i && in[beg_2 + i] == beg_1 + i)
			continue;
		if (in[beg_1 + i] >= 0)
			in[in[beg_1 + i]] = beg_2 + i;
		if (in[beg_2 + i] >= 0)
			in[in[beg_2 + i]] = beg_1 + i;
		std::swap(in[beg_1 + i], in[beg_2 + i]);
	}

	cdebug << " -> " << join(in.begin(), in.end(), "") << '\n';
}

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

bool is_tensor(Ex::iterator it)
{
	// Check whether it has children which are indices. If so, it is
	// a tensor
	for (auto beg = it.begin(), end = it.end(); beg != end; ++beg) {
		if (beg->is_index())
			return true;
	}
	return false;
}

void add_terms(terms_t& lhs, const terms_t& rhs, mpq_class factor)
{
	for (const auto& kv : rhs) {
		lhs[kv.first] += kv.second * factor;
		if (lhs[kv.first] == 0)
			lhs.erase(kv.first);
	}
}

bool has_tableau_symmetry(const Kernel& kernel, Ex::iterator it)
{
	if (*it->name == "\\prod") {
		for (Ex::sibling_iterator beg = it.begin(), end = it.end(); beg != end; ++beg) {
			if (!has_tableau_symmetry(kernel, beg))
				return false;
		}
		return true;
	}
	else if (is_tensor(it)) {
		auto tb = kernel.properties.get_composite<TableauBase>(it);
		return tb != nullptr;
	}
	else {
		return false;
	}
}

young_reduce::young_reduce(const Kernel& kernel, Ex& ex, const Ex& pat)
	: Algorithm(kernel, ex)
	, pat(pat)
	, index_map(std::make_unique<IndexMap>())
{
	cdebug << "#-----------------------------------------\n";
	cdebug << "# Calling young_reduce with\n";
	cdebug << "#   ex = " << to_string(ex, kernel) << '\n';
	cdebug << "#   pat = " << to_string(pat, kernel) << '\n';
	cdebug << "#-----------------------------------------\n";
	if (!has_tableau_symmetry(kernel, pat.begin()))
		throw std::runtime_error("Argument `pat` must have a TableauSymmetry");
}

young_reduce::~young_reduce()
{
}

bool young_reduce::can_apply(iterator it)
{
	if (*it->name == "\\sum") {
		for (Ex::sibling_iterator beg = it.begin(), end = it.end(); beg != end; ++beg) {
			if (can_apply(beg))
				return true;
		}
		return false;
	}
	else {
		return hash_compare(it, pat.begin(), HASH_IGNORE_INDEX_ORDER | HASH_IGNORE_TOP_MULTIPLIER);
	}
}

young_reduce::result_t young_reduce::apply_sum(iterator& it)
{
	std::vector<terms_t> decomps;
	std::vector<iterator> terms;

	for (Ex::sibling_iterator beg = it.begin(), end = it.end(); beg != end; ++beg) {
		if (!hash_compare(beg, pat.begin(), HASH_IGNORE_INDEX_ORDER | HASH_IGNORE_TOP_MULTIPLIER))
			continue;
		decomps.push_back(symmetrize(beg));
		terms.push_back(beg);
	}

	// Now that we have a bunch of decompositions, we should try and
	// get rid of the maximum number of terms possible. Begin by adding
	// all the terms together and seeing if that works, if not then
	// work through the combinations.
	for (size_t n_terms = decomps.size(); n_terms >= 1; --n_terms) {
		std::vector<int> mask(decomps.size(), 1);
		std::fill(mask.begin() + n_terms, mask.end(), 0);
		do {
			terms_t decomp;
			for (size_t i = 0; i < decomps.size(); ++i) {
				if (mask[i]) {
					add_terms(decomp, decomps[i], 1);
				}
			}

			if (decomp.empty()) { // The decomposition is zero
				cdebug << "Matched " << to_string(it, kernel) << " to 0\n";
				// Remove old terms
				for (size_t i = 0; i < terms.size(); ++i) {
					if (mask[i])
						tr.erase(terms[i]);
				}
				cleanup_dispatch(kernel, tr, it);
				return result_t::l_applied;
			}

			auto fact = linear_compare(decomp, pat_decomp);
			if (fact != 0) { // We found a match
				cdebug << "Matched " << to_string(it, kernel) << " to ";

				// Remove old terms
				for (size_t i = 0; i < terms.size(); ++i) {
					if (mask[i]) {
						cdebug << to_string(terms[i], kernel) << ", ";
						tr.erase(terms[i]);
					}
				}
				cdebug << '\n';
				// Add new term and give it the correct multiplier
				auto rep = tr.append_child(it, pat.begin());
				multiply(rep->multiplier, fact);
				return result_t::l_applied;
			}
		} while (std::next_permutation(mask.begin(), mask.end()));

	}
	return result_t::l_no_action;
}

young_reduce::result_t young_reduce::apply_monoterm(iterator& it)
{
	auto d = symmetrize(it);
	auto fact = linear_compare(d, pat_decomp);
	if (fact != 0) {
		cdebug << "Matched " << to_string(it, kernel) << " to " << to_string(pat, kernel) << '\n';
		it = tr.replace(it, pat.begin());
		multiply(it->multiplier, fact);
		cleanup_dispatch(kernel, tr, it);
		return result_t::l_applied;
	}
	else {
		return result_t::l_no_action;
	}
}

young_reduce::result_t young_reduce::apply(iterator& it)
{
	cdebug << "--- Applying to " << to_string(it, kernel) << "\n";
	// We symmetrize pattern now (lazily) to avoid an expensive
	// symmetrization in case of no nodes matching
	if (pat_decomp.empty())
		pat_decomp = symmetrize(pat.begin());

	result_t res;
	if (*it->name == "\\sum")
		res = apply_sum(it);
	else
		res = apply_monoterm(it);

	cdebug << "--- tree is now " << to_string(tr, kernel) << '\n';

	return res;
}

//----------------------------------------
terms_t young_reduce::symmetrize(Ex::iterator it)
{
	cdebug << "--- symmetrizing " << to_string(it, kernel) << ":\n";

	names_t names;
	indices_t indices;
	std::deque<Symmetry> symmetries;

	Ex::sibling_iterator beg, end;
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
			for (size_t row = 0; row < tab.number_of_rows(); ++row) {
				if (tab.row_size(row) > 1) {
					cdebug << "Found symmetry " << join(tab.begin_row(row), tab.end_row(row)) << '\n';
					symmetries.emplace_back(indices_t(tab.begin_row(row), tab.end_row(row)), false, indices.size());
				}
			}
			for (size_t col = 0; col < tab.row_size(0); ++col) {
				if (tab.column_size(col) > 1) {
					cdebug << "Found antisymmetry " << join(tab.begin_column(col), tab.end_column(col)) << '\n';
					symmetries.emplace_front(indices_t(tab.begin_column(col), tab.end_column(col)), true, indices.size());
				}
			}
		}

		size_t n_indices = 0;
		//Iterate over children to populate `indices`
		for (auto idx = beg.begin(), edx = beg.end(); idx != edx; ++idx) {
			if (idx->is_index()) {
				++n_indices;
				cdebug << "Found index " << *idx->name << '\n';
				indices.push_back(index_map->name_to_idx(idx->name));
			}
		}
		// Populate `names`
		cdebug << "Added " << n_indices << " to " << *beg->name << '\n';
		names.emplace_back(beg->name, n_indices);

		++beg;
	}

	cdebug << "Indices: " << join(indices.begin(), indices.end()) << '\n';

	indices_t identity(indices.size(), -1);
	for (size_t i = 0; i < identity.size(); ++i) {
		if (identity[i] >= 0) {
			continue;
		}
		size_t pos = std::distance(indices.begin(), std::find(indices.begin() + i + 1, indices.end(), indices[i]));
		if (pos == indices.size()) {
			identity[i] = -indices[i];
		}
		else {
			identity[i] = static_cast<index_t>(pos);
			identity[pos] = static_cast<index_t>(i);
		}
	}

	cdebug << "PII: " << join(identity.begin(), identity.end()) << '\n';

	terms_t terms;
	terms[identity] = *it->multiplier;

	// Symmetrize in identical tensors
	index_t pos_i = 0;
	for (size_t i = 0; i < names.size(); ++i) {
		index_t pos_j = pos_i + (index_t)names[i].second;
		for (size_t j = i + 1; j < names.size(); ++j) {
			if (names[i] == names[j]) {
				terms_t new_terms;
				for (const auto& term : terms) {
					// Make new term
					auto new_term = term.first;
					// Apply symmetry
					swap_indices(new_term, pos_i, pos_j, (index_t)names[i].second);
					// Insert
					new_terms[new_term] = term.second;
				}
				terms = new_terms;
			}
		}
		pos_i += (index_t)names[i].second;
	}

	cdebug << "Symmetrized in identical tensors:\n";
	for (const auto& kv : terms)
		cdebug << '\t' << join(kv.first.begin(), kv.first.end()) << '\t' << kv.second << '\n';


	// Young project
	size_t n_terms = terms.size();
	for (auto symmetry : symmetries) {
		cdebug << "Applying " << (symmetry.antisymmetric ? "anti" : "") << "symmetry " << join(symmetry.indices.begin(), symmetry.indices.end());
		terms_t new_terms;
		for (const auto& term : terms) {
			// Divide contribution
			new_terms[term.first] += term.second;
			// Make new terms
			symmetry.reset();
			while (symmetry.next()) {
				auto new_term = symmetry.apply(term.first);
				new_terms[new_term.first] += new_term.second * term.second;
				if (new_terms[new_term.first] == 0) {
					new_terms.erase(new_term.first);
				}
			}
		}
		terms = new_terms;
		cdebug << "(generated " << (terms.size() - n_terms) << " terms)\n";
		n_terms = terms.size();
	}

	cdebug << "Projected:\n";
	for (const auto& kv : terms)
		cdebug << '\t' << join(kv.first.begin(), kv.first.end()) << '\t' << kv.second << '\n';
	cdebug << "\n";

	return terms;
}
