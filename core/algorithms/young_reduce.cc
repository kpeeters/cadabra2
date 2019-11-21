#include <iostream>
#include <map>
#include <vector>
#include <algorithm>

#include "Compare.hh"
#include "Cleanup.hh"
#include "algorithms/young_reduce.hh"
#include "properties/TableauSymmetry.hh"

/////////////////////////////////////////////////////////////////////

#include <sstream>
#include "DisplayTerminal.hh"

std::string ex_to_string(cadabra::Ex ex, const cadabra::Kernel& kernel)
{
	cadabra::DisplayTerminal dt(kernel, ex, true);
	std::stringstream ss;
	dt.output(ss);
	return "$" + ss.str() + "$";
}

std::string ex_to_string(cadabra::Ex::iterator it, const cadabra::Kernel& kernel)
{
	return ex_to_string(cadabra::Ex(it), kernel);
}

std::string adjform_to_string(const cadabra::yr::adjform_t& adjform, const std::vector<cadabra::nset_t::iterator>& index_map)
{
	std::map<cadabra::yr::index_t, int> dummy_map;
	int dummy_counter = 0;
	std::string res;
	for (const auto& elem : adjform) {
		if (elem < 0) {
			res += *index_map[-(elem + 1)];
		}
		else if (dummy_map.find(elem) != dummy_map.end()) {
			res += "d_" + std::to_string(dummy_map[elem]);
		}
		else {
			dummy_map[adjform[adjform[elem]]] = dummy_counter++;
			res += "d_" + std::to_string(dummy_map[adjform[adjform[elem]]]);
		}
	}
	return res;
}

std::string pf_to_string (const cadabra::yr::ProjectedForm& projform, const std::vector<cadabra::nset_t::iterator>& index_map)
{
	std::stringstream os;
	int i = 0;
	int max = 20;
	auto it = projform.data.begin();
	while (i < max && i < projform.data.size()) {
		for (const auto& elem : it->first)
			os << elem << ' ';
		os << '\t' << it->second << '\n';
		++i;
		++it;
	}
	if (i == max) {
		os << "(skipped " << (projform.data.size() - max) << " terms)\n";
	}
	return os.str();
}

#define DEBUG_OUTPUT 1
#define cdebug if (!DEBUG_OUTPUT) {} else std::cerr


////////////////////////////////////////////////////////////////////

// Returns the position of 'val' between 'begin' and 'end', starting
// the search at 'offset'
template <typename It, typename T>
size_t index_of(It begin, It end, const T& val, size_t offset = 0)
{
	auto pos = std::find(begin + offset, end, val);
	return std::distance(begin, pos);
}

namespace cadabra {
	namespace yr
	{
		mpq_class ProjectedForm::compare(const ProjectedForm& other) const
		{
			cdebug << "entered compare\n";
			// Early failure checks
			if (data.empty() || data.size() != other.data.size())
				return 0;

			// Find the numeric factor between the first two terms, then loop over all
			// other terms checking that the factor is the same. If not, return 0
			auto a_it = data.begin(), b_it = other.data.begin(), a_end = data.end();
			mpq_class factor = a_it->second / b_it->second;
			cdebug << "factor is " << factor << '\n';
			while (a_it != a_end) {
				cdebug << "comparing " << a_it->second << " * ";
				for (const auto& elem : a_it->first)
					cdebug << elem << ' ';
				cdebug << " to " << b_it->second << " * ";
				for (const auto& elem : b_it->first)
					cdebug << elem << ' ';
				cdebug << '\n';
				if (a_it->second / b_it->second != factor) {
					cdebug << "factor was " << (a_it->second / b_it->second) << "!\n";
					return 0;
				}
				++a_it, ++b_it;
			}
			cdebug << "matched all terms!\n";
			return factor;
		}

		void ProjectedForm::combine(const ProjectedForm& other, mpq_class factor)
		{
			if (factor == 1) {
				for (const auto& kv : other.data) {
					data[kv.first] += kv.second;
					if (data[kv.first] == 0)
						data.erase(kv.first);
				}
			}
			else {
				for (const auto& kv : other.data) {
					data[kv.first] += kv.second * factor;
					if (data[kv.first] == 0)
						data.erase(kv.first);
				}
			}
		}

		void ProjectedForm::multiply(mpq_class k)
		{
			for (auto& kv : data)
				kv.second *= k;
		}

		void ProjectedForm::clear()
		{
			data.clear();
		}

		void ProjectedForm::insert(adjform_t adjform, mpq_class value)
		{
			data[adjform] = value;
		}

		void ProjectedForm::apply_young_symmetry(const std::vector<index_t>& indices, bool antisymmetric)
		{
			map_t old_data = data;

			// Loop over all entries, for each one looping over all permutations
			// of the indices to be symmetrized and creating a new term for that
			// permutation; then add the new term to the list of entries
			for (const auto& kv : old_data) {
				auto perm = indices;
				bool flip = false;
				int parity = 1;
				while (std::next_permutation(perm.begin(), perm.end())) {
					if (antisymmetric && (flip = !flip))
						parity *= -1;
					auto ret = kv.first;
					for (size_t i = 0; i < indices.size(); ++i) {
						ret[indices[i]] = kv.first[perm[i]];
					}
					for (auto index : indices) {
						if (ret[index] >= 0)
							ret[ret[index]] = index;
					}
					data[ret] += parity * kv.second;
				}
			}
		}

		void ProjectedForm::apply_ident_symmetry(std::vector<index_t> positions, index_t n_indices)
		{
			map_t old_data = data;
			std::sort(positions.begin(), positions.end());
			auto perm = positions;

			// Loop over all entries, for each loop over all permutations
			// of identical symbol and create a new term for that permutation;
			// then add the new term to the list of entries
			for (const auto& kv : old_data) {
				while (std::next_permutation(perm.begin(), perm.end())) {
					auto term = collapse_dummy_indices(kv.first);
					adjform_t out = term;
					for (size_t i = 0; i < perm.size(); ++i) {
						for (index_t k = 0; k < n_indices; ++k) {
							out[perm[i] + k] = term[positions[i] + k];
						}
					}
					data[expand_dummy_indices(out)] += kv.second;
				}
			}
		}

		bool check_structure(Ex::iterator lhs, Ex::iterator rhs)
		{
			// Early failure checks
			if (lhs->name != rhs->name) {
				return false;
			}

			Ex::iterator l1 = lhs.begin(), l2 = lhs.end();
			Ex::iterator r1 = rhs.begin(), r2 = rhs.end();

			std::vector<Ex::iterator> l_indices, r_indices;

			// Loop over all tree nodes using a depth first iterator. If the
			// entry is an index ensure that it has the same parent_rel, if it
			// is any other type of node check that the names match.
			while (l1 != l2 && r1 != r2) {
				if (l1->is_index()) {
					l1.skip_children();
					r1.skip_children();
					if (l1->fl.parent_rel != r1->fl.parent_rel) {
						return false;
					}
					l_indices.push_back(l1);
					r_indices.push_back(r1);
				}
				else {
					if (l1->name != r1->name || l1->multiplier != r1->multiplier) {
						return false;
					}
				}
				++l1, ++r1;
			}

			return l1 == l2 && r1 == r2;
		}

		std::vector<Ex::iterator> split_ex(Ex::iterator it, const std::string& delim)
		{
			if (*it->name == delim) {
				// Loop over children creating a list
				std::vector<Ex::iterator> res;
				Ex::sibling_iterator beg = it.begin(), end = it.end();
				while (beg != end) {
					res.push_back(beg);
					++beg;
				}
				return res;
			}
			else {
				// Return a list containing only 'it'
				return std::vector<Ex::iterator>(1, it);
			}
		}

		std::vector<Ex::iterator> split_ex(Ex::iterator it, const std::string& delim, Ex::iterator pat)
		{
			if (*it->name == delim) {
				std::vector<Ex::iterator> res;
				Ex::sibling_iterator beg = it.begin(), end = it.end();
				while (beg != end) {
					if (check_structure(beg, pat))
						res.push_back(beg);
					++beg;
				}
				return res;
			}
			else {
				if (check_structure(it, pat))
					return std::vector<Ex::iterator>(1, it);
				else
					return std::vector<Ex::iterator>();
			}
		}

		adjform_t collapse_dummy_indices(adjform_t adjform)
		{
			index_t next_free_index = adjform.size();
			for (size_t i = 0; i < adjform.size(); ++i) {
				if (adjform[i] >= 0 && adjform[i] < (index_t)adjform.size()) {
					adjform[adjform[i]] = next_free_index;
					adjform[i] = next_free_index;
					++next_free_index;
				}
			}
			return adjform;
		}

		adjform_t expand_dummy_indices(adjform_t adjform)
		{
			for (size_t idx = 0; idx < adjform.size(); ++idx) {
				if (adjform[idx] >= (index_t)adjform.size()) {
					auto pos = index_of(adjform.begin(), adjform.end(), adjform[idx], idx + 1);
					adjform[idx] = pos;
					adjform[pos] = idx;
				}
			}
			return adjform;
		}
	}
}

using namespace cadabra;
using namespace yr;

young_reduce::young_reduce(const Kernel& kernel, Ex& ex, const Ex* pattern)
	: Algorithm(kernel, ex)
{
	if (pattern) {
		if (!set_pattern(pattern->begin()))
			throw std::runtime_error("Tried to construct young_reduce object with invalid pattern");
	}
}

young_reduce::~young_reduce()
{

}

bool young_reduce::can_apply(iterator it)
{
	return true;
}

young_reduce::result_t young_reduce::apply(iterator& it)
{
	result_t res;

	if (pat == Ex::iterator()) {
		res = apply_unknown(it);
	}
	else {
		res = apply_known(it);
	}

	if (res != result_t::l_no_action) {
		cdebug << "Action taken; cleaning...";
		cleanup_dispatch(kernel, tr, it);
		cdebug << "done!\n";
	}
	return res;
}

young_reduce::result_t young_reduce::apply_known(iterator& it)
{
	cdebug << "Apply known:\n\tpat = " << ex_to_string(pat, kernel) << "\n\t it = " << ex_to_string(it, kernel) << '\n';
	ProjectedForm it_sym;
	auto nodes = split_ex(it, "\\sum", pat);
	cdebug << "Found " << nodes.size() << " terms which match pat:\n";
	if (nodes.size() == 0)
		return result_t::l_no_action;

	for (auto& node : nodes) {
		cdebug << "\t" << ex_to_string(node, kernel) << "\n";
		if (subtree_equal(&kernel.properties, pat, node, -2, true, -1)) {
			cdebug << "Matched pat; combining\n";
			it_sym.combine(pat_sym, *node->multiplier / *pat->multiplier);
		}
		else {
			it_sym.combine(symmetrize(node));
		}
	}

	// Check if projection yielded zero
	if (it_sym.data.empty()) {
		cdebug << "Projection yielded 0; zeroing node\n";
		node_zero(it);
		return result_t::l_applied;
	}

	// Check if projection is a multiple of 'pat'
	auto factor = it_sym.compare(pat_sym);
	if (factor != 0) {
		cdebug << "Projection was a multiple (" << factor << ") of pat; reducing...\n";
		it = tr.replace(nodes.back(), pat);
		nodes.pop_back();
		for (auto node : nodes)
			node_zero(node);
		multiply(it->multiplier, factor);
		cdebug << "Produced " << ex_to_string(it, kernel) << '\n';
		return result_t::l_applied;
	}
	else {
		cdebug << "No match found\n";
		return result_t::l_no_action;
	}
}

young_reduce::result_t young_reduce::apply_unknown(iterator& it)
{
	cdebug << "Apply unknown:\n\t it = " << ex_to_string(it, kernel) << '\n';

	result_t res = result_t::l_no_action;

	auto terms = split_ex(it, "\\sum");
	cdebug << "Found " << terms.size() << " terms:\n";
	for (auto term : terms)
		cdebug << "\t" << ex_to_string(term, kernel) << "\n";
	while (!terms.empty()) {
		cdebug << "Examining " << ex_to_string(terms.back(), kernel) << "...";
		bool can_reduce = set_pattern(terms.back());
		if (can_reduce && pat_sym.data.empty()) {
			cdebug << "pat_sym is zero; zeroing node\n";
			node_zero(pat);
			res = result_t::l_applied;
			continue;
		}
		std::vector<Ex::iterator> cur_terms;
		for (index_t i = terms.size() - 2; i != -1; --i) {
			if (check_structure(terms.back(), terms[i])) {
				cur_terms.push_back(terms[i]);
				terms.erase(terms.begin() + i);
			}
		}
		terms.pop_back();
		cdebug << "found " << cur_terms.size() << " matching term(s):\n";
		for (auto term : cur_terms)
			cdebug << "\t" << ex_to_string(term, kernel) << '\n';
		if (can_reduce && !cur_terms.empty()) {
			cdebug << "Pat is (potentially) reduceable...\n";
			ProjectedForm it_sym;
			for (auto& node : cur_terms)
				it_sym.combine(symmetrize(node));

			if (it_sym.data.empty()) {
				cdebug << "Projection yielded 0; zeroing nodes\n";
				for (auto node : cur_terms)
					node_zero(node);
				res = result_t::l_applied;
			}
			else {
				auto factor = it_sym.compare(pat_sym);
				if (factor != 0) {
					cdebug << "Projection was a multiple of pat (factor " << factor << "); reducing...\n";
					multiply(pat->multiplier, factor + 1);
					for (auto& node : cur_terms)
						node_zero(node);
					cdebug << "Now have " << ex_to_string(it, kernel) << '\n';
					res = result_t::l_applied;
				}
				else {
					cdebug << "No match found...\n";
				}
			}
		}
		else {
			cdebug << "Pat is not reduceable, skipping...\n";
		}
	}

	pat = Ex::iterator();
	pat_sym.clear();
	return res;
}

bool young_reduce::set_pattern(Ex::iterator new_pat)
{
	cdebug << "Setting pattern to " << ex_to_string(new_pat, kernel) << '\n';
	pat = Ex::iterator();
	pat_sym.clear();

	auto collect = split_ex(new_pat, "\\prod");
	if (collect.empty())
		throw std::runtime_error("pat is empty");

	cdebug << "Checking for TableauBase property...";
	bool has_tableau_symmetry = false;
	for (const auto& term: collect) {
		if (kernel.properties.get_composite<TableauBase>(term) != nullptr) {
			has_tableau_symmetry = true;
			break;
		}
	}
	cdebug << (has_tableau_symmetry ? "true!" : "false - exiting...") << '\n';

	if (!has_tableau_symmetry) 
		return false;

	pat = new_pat;
	pat_sym = symmetrize(new_pat);	

	return true;	
}

ProjectedForm young_reduce::symmetrize(Ex::iterator it)
{
	cdebug << "symmetrizing " << ex_to_string(it, kernel) << "produces:\n";
	ProjectedForm sym;
	sym.insert(to_adjform(it), 1);

	// Symmetrize in identical tensors
	std::map<std::string, std::pair<index_t, std::vector<index_t>>> idents;
	index_t pos = 0;
	auto terms = split_ex(it, "\\prod");
	for (auto& term : terms) {
		idents[*term->name].first = term.number_of_children();
		idents[*term->name].second.push_back(pos);
		pos += term.number_of_children();
	}

	for (const auto& ident : idents) {
		if (ident.second.second.size() == 1)
			continue;
		sym.apply_ident_symmetry(ident.second.second, ident.second.first);
	}

	// Young project antisymmetric components
	pos = 0;
	for (auto& it : terms) {
		auto tb = kernel.properties.get_composite<TableauBase>(it);
		auto tab = tb->get_tab(kernel.properties, tr, it, 0);
		for (size_t col = 0; col < tab.row_size(0); ++col) {
			if (tab.column_size(col) > 1) {
				std::vector<index_t> indices;
				for (auto beg = tab.begin_column(col), end = tab.end_column(col); beg != end; ++beg)
					indices.push_back(*beg + pos);
				std::sort(indices.begin(), indices.end());
				sym.apply_young_symmetry(indices, true);
			}
		}
		pos += it.number_of_children();
	}

	// Young project symmetric components
	pos = 0;
	for (auto& it : terms) {
		// Apply the symmetries
		auto tb = kernel.properties.get_composite<TableauBase>(it);
		auto tab = tb->get_tab(kernel.properties, tr, it, 0);
		for (size_t row = 0; row < tab.number_of_rows(); ++row) {
			if (tab.row_size(row) > 1) {
				std::vector<index_t> indices;
				for (auto beg = tab.begin_row(row), end = tab.end_row(row); beg != end; ++beg)
					indices.push_back(*beg + pos);
				std::sort(indices.begin(), indices.end());
				sym.apply_young_symmetry(indices, false);
			}
		}
		pos += it.number_of_children();
	}

	sym.multiply(*it->multiplier);

	cdebug << sym << '\n';

	return sym;
}

adjform_t young_reduce::to_adjform(Ex::iterator it)
{
	adjform_t adjform;
	size_t pos = 0;
	for (Ex::iterator beg = it.begin(), end = it.end(); beg != end; ++beg) {
		if (!beg->is_index())
			continue;

		// Only fill in if it hasn't been yet
		if (adjform.size() <= pos || adjform[pos] < 0) {
			// Attempt to find a matching dummy index
			bool found = false;
			size_t searchpos = pos + 1;
			for (Ex::iterator cur = std::next(beg); cur != end; ++cur) {
				if (!cur->is_index())
					continue;
				if (beg->name == cur->name) {
					// Make sure vector is big enough
					if (adjform.size() <= searchpos)
						adjform.resize(searchpos + 1, -1);
					adjform[pos] = searchpos;
					adjform[searchpos] = pos;
					found = true;
					break;
				}
				++searchpos;
			}

			// No matching dummy index found, add as a free index
			if (!found) {
				if (adjform.size() <= pos)
					adjform.resize(pos + 1, -1);
				adjform[pos] = get_free_index(beg->name);
			}
		}
		++pos;
	}

	return adjform;
}

index_t young_reduce::get_free_index (nset_t::iterator name)
{
	auto pos = index_of(index_map.begin(), index_map.end(), name);
	if (pos == index_map.size()) {
		index_map.push_back(name);
		return -(index_t)index_map.size();
	}
	else {
		return -(index_t)pos - 1;
	}
}
