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

#define DEBUG_OUTPUT 0
#define cdebug if (!DEBUG_OUTPUT) {} else std::cerr


////////////////////////////////////////////////////////////////////

using namespace cadabra;

bool check_structure(Ex::iterator lhs, Ex::iterator rhs)
{
	// Early failure checks
	if (lhs->name != rhs->name) {
		return false;
	}

	Ex::iterator l1 = lhs.begin(), l2 = lhs.end();
	Ex::iterator r1 = rhs.begin(), r2 = rhs.end();

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

bool has_TableauBase(Ex::iterator it, const cadabra::Kernel& kernel)
{
	if (*it->name == "\\prod" || *it->name == "\\sum") {
		for (Ex::sibling_iterator beg = it.begin(), end = it.end(); beg != end; ++beg)
			if (has_TableauBase(beg, kernel))
				return true;
		return false;
	}
	else {
		return (kernel.properties.get_composite<cadabra::TableauBase>(it) != nullptr);
	}
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
	if (pat == Ex::iterator()) {
		// check for TableauBase
		return has_TableauBase(it, kernel);
	}
	else
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
	AdjformEx it_sym;
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
	if (it_sym.empty()) {
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
		if (can_reduce && pat_sym.empty()) {
			cdebug << "pat_sym is zero; zeroing node\n";
			node_zero(pat);
			res = result_t::l_applied;
			continue;
		}
		std::vector<Ex::iterator> cur_terms;
		for (AdjformIdx i = terms.size() - 2; i != -1; --i) {
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
			AdjformEx it_sym;
			for (auto& node : cur_terms)
				it_sym.combine(symmetrize(node));

			if (it_sym.empty()) {
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
	if (!has_TableauBase(new_pat, kernel)) {
		cdebug << "false, returning...\n";
		return false;
	}
	else {
		cdebug << "true!\n";
	}

	pat = new_pat;
	pat_sym = symmetrize(new_pat);	

	return true;	
}

AdjformEx young_reduce::symmetrize(Ex::iterator it)
{
	cdebug << "symmetrizing " << ex_to_string(it, kernel) << "produces:\n";
	AdjformEx sym;
	sym.set(index_map.to_adjform(it), 1);

	// Symmetrize in identical tensors
	std::map<std::string, std::pair<size_t, std::vector<size_t>>> idents;
	size_t pos = 0;
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
		if(tb) {
			auto tab = tb->get_tab(kernel.properties, tr, it, 0);
			for (size_t col = 0; col < tab.row_size(0); ++col) {
				if (tab.column_size(col) > 1) {
					std::vector<size_t> indices;
					for (auto beg = tab.begin_column(col), end = tab.end_column(col); beg != end; ++beg)
						indices.push_back(*beg + pos);
					std::sort(indices.begin(), indices.end());
					sym.apply_young_symmetry(indices, true);
					}
				}
			}
		pos += it.number_of_children();
	}

	// Young project symmetric components
	pos = 0;
	for (auto& it : terms) {
		// Apply the symmetries
		auto tb = kernel.properties.get_composite<TableauBase>(it);
		if(tb) {
			auto tab = tb->get_tab(kernel.properties, tr, it, 0);
			for (size_t row = 0; row < tab.number_of_rows(); ++row) {
				if (tab.row_size(row) > 1) {
					std::vector<size_t> indices;
					for (auto beg = tab.begin_row(row), end = tab.end_row(row); beg != end; ++beg)
						indices.push_back(*beg + pos);
					std::sort(indices.begin(), indices.end());
					sym.apply_young_symmetry(indices, false);
					}
				}
			}
		pos += it.number_of_children();
		}
	
	sym.multiply(*it->multiplier);
	return sym;
}

#include "properties/Trace.hh"

bool has_Trace(const Kernel& kernel, Ex::iterator it)
{
	auto p = kernel.properties.get_composite<cadabra::Trace>(it);
	if (p)
		return true;
	else
		return false;
}


young_reduce_trace::young_reduce_trace(const Kernel& kernel, Ex& ex)
	: Algorithm(kernel, ex)
{

}

young_reduce_trace::~young_reduce_trace()
{

}

bool young_reduce_trace::can_apply(iterator it)
{
	// Accept sum nodes and trace nodes
	return has_Trace(kernel, it) || *it->name == "\\sum";
}

young_reduce_trace::result_t young_reduce_trace::apply(iterator& it)
{
	if (has_Trace(kernel, it)) {
		return apply_trace(it);
	}
	else {
		return apply_sum(it);
	}
}

young_reduce_trace::result_t young_reduce_trace::apply_sum(iterator& it)
{
	assert(*it->name == "\\sum");
	return result_t::l_no_action;
}

young_reduce_trace::result_t young_reduce_trace::apply_trace(iterator& it)
{
	assert(has_Trace(kernel, it));

	// Can only apply if the child node is a sum node
	if (it.number_of_children() == 0 || *it.begin()->name != "\\sum")
		return result_t::l_no_action;

	sibling_iterator beg = it.begin().begin(), end = it.begin().end();

	AdjformEx a1, a2;
	a1.add(index_map.to_adjform(beg));
	a1.apply_cyclic_symmetry();
	std::cerr << a1 << '\n';
	++beg;
	while (beg != end) {
		a2.add(index_map.to_adjform(beg));
		++beg;
	}

	a2.apply_cyclic_symmetry();
	std::cerr << a2 << '\n';

	auto factor = a1.compare(a2);
	std::cerr << "factor was " << factor << "\n";
	return result_t::l_no_action;
}
