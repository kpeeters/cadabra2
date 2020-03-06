#include <vector>
#include "properties/Trace.hh"
#include "properties/TableauBase.hh"
#include "Cleanup.hh"
#include "meld.hh"

using namespace cadabra;

//-------------------------------------------------
// generic useful routines

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

bool has_TableauBase(const cadabra::Kernel& kernel, Ex::iterator it)
{
	if (*it->name == "\\prod" || *it->name == "\\sum") {
		for (Ex::sibling_iterator beg = it.begin(), end = it.end(); beg != end; ++beg)
			if (has_TableauBase(kernel, beg))
				return true;
		return false;
	}
	else {
		return (kernel.properties.get_composite<cadabra::TableauBase>(it) != nullptr);
	}
}

bool has_Trace(const Kernel& kernel, Ex::iterator it)
{
	auto p = kernel.properties.get_composite<cadabra::Trace>(it);
	if (p)
		return true;
	else
		return false;
}

//-------------------------------------------------
// meld stuff

meld::meld(const Kernel& kernel, Ex& ex)
    : Algorithm(kernel, ex)
{

}

meld::~meld()
{

}

bool meld::can_apply(iterator it)
{
    return
        *it->name == "\\sum" ||
        has_Trace(kernel, it) ||
        has_TableauBase(kernel, it);

}

#define APPLY_ROUTINE(name)             \
    switch (name (it)) {                \
        case  result_t::l_applied:      \
            res = result_t::l_applied;  \
            break;                      \
        case result_t::l_error:         \
            return result_t::l_applied; \
        default:                        \
            break;                      \
    }                                   //end

meld::result_t meld::apply(iterator& it) 
{
    result_t res = result_t::l_no_action;

	APPLY_ROUTINE(do_traces);
    APPLY_ROUTINE(do_tableaux);
    
	cleanup(it);
    cleanup_dispatch(kernel, tr, it);

    return res;
}

void meld::cleanup(iterator it)
{
    // Remove empty traces    
    if (has_Trace(kernel, it) && it.number_of_children() == 0) {
		node_zero(it);
	}
	else if (*it->name == "\\sum" || *it->name == "\\prod") {
		for (Ex::sibling_iterator beg = it.begin(), end = it.end(); beg != end; ++beg) {
            Ex::iterator newit = beg;
			cleanup(newit);
		}
	}
}

//-------------------------------------------------
// do_tableaux stuff

AdjformEx meld::symmetrize(Ex::iterator it)
{
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

meld::result_t meld::do_tableaux(iterator it)
{
	result_t res = result_t::l_no_action;

	auto terms = split_ex(it, "\\sum");
	while (!terms.empty()) {
        auto pat = terms.back();
        bool can_reduce = has_TableauBase(kernel, pat);
        auto collect = split_ex(pat, "\\prod");
		auto pat_sym = symmetrize(pat);

		if (can_reduce && pat_sym.empty()) {
			node_zero(pat);
			res = result_t::l_applied;
			continue;
		}
		std::vector<Ex::iterator> cur_terms;
		for (AdjformIdx i = terms.size() - 2; i != -1; --i) {
			if (check_structure(pat, terms[i])) {
				cur_terms.push_back(terms[i]);
				terms.erase(terms.begin() + i);
			}
		}
		terms.pop_back();
		if (can_reduce && !cur_terms.empty()) {
			AdjformEx it_sym;
			for (auto& node : cur_terms)
				it_sym.combine(symmetrize(node));

			if (it_sym.empty()) {
				for (auto node : cur_terms)
					node_zero(node);
				res = result_t::l_applied;
			}
			else {
				auto factor = it_sym.compare(pat_sym);
				if (factor != 0) {
					multiply(pat->multiplier, factor + 1);
					for (auto& node : cur_terms)
						node_zero(node);
					res = result_t::l_applied;
				}
			}
		}
	}
	return res;
}

//-------------------------------------------------
// do_trace stuff

void cycle_adjform(Adjform& adjform, size_t n)
{
	if (adjform.size() < 2)
		return;
	n %= adjform.size();

	std::rotate(adjform.begin(), adjform.end() - n, adjform.end());
	for (auto& idx : adjform) {
		if (idx >= 0)
			idx = (idx + n) % adjform.size();
	}
}

void cycle_vec(std::vector<size_t>& vec, size_t n)
{
	n %= vec.size();
	std::rotate(vec.begin(), vec.end() - n, vec.end());
}

struct TraceTerm
{ 
    TraceTerm(Ex::iterator it, mpq_class parent_multiplier, IndexMap& index_map);
    Ex::iterator it;
    Adjform names, indices; 
    mpq_class parent_multiplier;
    std::vector<size_t> pushes;
};

TraceTerm::TraceTerm(Ex::iterator it, mpq_class parent_multiplier, IndexMap& index_map)
	: it(it)
	, parent_multiplier(parent_multiplier)
{
    auto terms = split_ex(it, "\\prod");
    for (const auto& term : terms) {
        names.push_back(index_map.get_free_index(term->name));
        pushes.push_back(0);
        for (Ex::sibling_iterator beg = term.begin(), end = term.end(); beg != end; ++beg) {
            if (beg->is_index()) {
				indices.push_back(index_map.get_free_index(term->name));
                ++pushes.back();
			}
        }
    }
}

std::vector<TraceTerm> collect_trace_terms(Ex::iterator it, const Kernel& kernel, IndexMap& index_map)
{
    // If a trace node just return all the children
	if (has_Trace(kernel, it)) {
		Ex::sibling_iterator beg = it.begin(), end = it.end();
		if (beg == end)
			return {};
		if (*beg->name != "\\sum")
			return { TraceTerm(beg, *it->multiplier, index_map) };
		std::vector<TraceTerm> ret;
		for (Ex::sibling_iterator a = beg.begin(), b = beg.end(); a != b; ++a)
			ret.emplace_back(a, *it->multiplier, index_map);
		return ret;
	}
	
	// If a sum node, collect all trace nodes
	if (*it->name == "\\sum") {
		std::vector<TraceTerm> ret;
		for (Ex::sibling_iterator a = it.begin(), b = it.end(); a != b; ++a) {
			if (has_Trace(kernel, a)) {
				auto nodes = collect_trace_terms(a, kernel, index_map);
				ret.insert(ret.end(), nodes.begin(), nodes.end());
			}
		}
		return ret;
	}

	// Else return nothing
	return {};
}

meld::result_t meld::do_traces(iterator it)
{
    auto terms = collect_trace_terms(it, kernel, index_map);
	if (terms.empty())
		return result_t::l_no_action;

	auto res = result_t::l_no_action;
	for (size_t i = 0; i < terms.size() - 1; ++i) {
		for (size_t j = i + 1; j < terms.size(); ++j) {
			auto perm = terms[j];
			do {
				if (terms[i].names == perm.names && terms[i].indices == perm.indices) {
					multiply(terms[i].it->multiplier, 1 + ((terms[j].parent_multiplier * *terms[j].it->multiplier) / (terms[i].parent_multiplier * *terms[i].it->multiplier)));
					tr.erase(terms[j].it);
					terms.erase(terms.begin() + j);
					--j;
					res= result_t::l_applied;
					break;
				}

				cycle_adjform(perm.names, 1);
				cycle_adjform(perm.indices, perm.pushes.back());
				cycle_vec(perm.pushes, 1);
			} while (perm.names != terms[j].names || perm.indices != terms[j].indices);
		}
	}
	return res;
}
