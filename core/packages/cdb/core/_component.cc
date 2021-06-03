#include <pybind11/pybind11.h>
#include "pythoncdb/py_ex.hh"
#include "pythoncdb/py_kernel.hh"

#include "Exceptions.hh"
#include "properties/Coordinate.hh"
#include "properties/IndexInherit.hh"
#include "properties/Symbol.hh"

using namespace cadabra;
namespace py = pybind11;

std::vector<Ex> get_free_indices(const Kernel& kernel, Ex::iterator it)
{
	while (*it->name == "\\sum" || *it->name == "\\equals") {
		bool found_term = false;
		for (Ex::sibling_iterator beg = it.begin(), end = it.end(); beg != end; ++beg) {
			if (*beg->name != "1") {
				found_term = true;
				it = beg;
				break;
			}
		}
		if (!found_term)
			return {};
	}

	std::vector<Ex> indices;
	bool inherits_indices = kernel.properties.get<IndexInherit>(it);
	for (Ex::sibling_iterator beg = it.begin(), end = it.end(); beg != end; ++beg) {
		if (beg->is_index()) {
			auto symbol = kernel.properties.get<Symbol>(beg, true);
			auto coord = kernel.properties.get<Coordinate>(beg, true);
			if (!(symbol || coord || beg->is_integer())) {
				Ex idx(beg);
				idx.begin()->fl.parent_rel = str_node::parent_rel_t::p_sub;
				auto pos = std::find(indices.begin(), indices.end(), idx);
				if (pos == indices.end())
					indices.push_back(idx);
				else
					indices.erase(pos);
			}
		}
		else if (inherits_indices) {
			auto new_indices = get_free_indices(kernel, beg);
			for (const auto& idx : new_indices) {
				auto pos = std::find(indices.begin(), indices.end(), idx);
				if (pos == indices.end())
					indices.push_back(idx);
				else
					indices.erase(pos);
			}
		}
	}

	return indices;
}

Ex_ptr get_component(Ex_ptr ex, Ex_ptr components)
{
	Kernel& kernel = *get_kernel_from_scope();

	// Ensure components is a comma node
	if (*components->begin()->name != "\\comma") {
		Ex_ptr new_components = std::make_shared<Ex>("\\comma");
		new_components->append_child(new_components->begin(), components->begin());
		components = new_components;
	}

	// Resolve component nodes
	for (Ex::iterator beg = ex->begin(), end = ex->end(); beg != end; ++beg) {
		if (*beg->name == "\\components") {
			Ex::sibling_iterator comma = beg.begin();
			++comma;
			bool found = false;
			for (Ex::sibling_iterator compbeg = comma.begin(), compend = comma.end(); compbeg != compend; ++compbeg) {
				Ex::sibling_iterator side = compbeg.begin();
				if (subtree_equal(&kernel.properties, side, components->begin())) {
					++side;
					Ex tmp(side);
					beg = ex->replace(beg, tmp.begin());
					found = true;
					break;
				}
			}
			if (!found) {
				beg = ex->replace(beg, str_node("1"));
				multiply(beg->multiplier, 0);
			}
		}
	}

	// Resolve free indices
	auto free_indices = get_free_indices(kernel, ex->begin());
	if (free_indices.empty())
		return ex;

	// Match free indices to coordinates
	Ex::sibling_iterator curcomp = components->begin().begin(), endcomp = components->begin().end();
	std::map<Ex, Ex::iterator> idx_to_coord;
	for (const auto& free_index : free_indices) {
		if (curcomp == endcomp)
			throw ArgumentException("get_component: number of components does not match number of free indices");
		idx_to_coord[free_index] = curcomp;
		++curcomp;
	}

	// Iterate through indices replacing free indices with coordinates
	Ex_comparator comp(kernel.properties);
	auto beg = index_iterator::begin(kernel.properties, ex->begin());
	auto end = index_iterator::end(kernel.properties, ex->begin());
	while (beg != end) {
		auto next = beg;
		++next;

		for (const auto& elem : idx_to_coord) {
			comp.clear();
			auto compres = comp.equal_subtree(beg, elem.first.begin(), Ex_comparator::useprops_t::always, true);
			if (compres == Ex_comparator::match_t::subtree_match) {
				ex->replace_index(beg, elem.second, true);
				break;
			}
		}
		beg = next;
	}

	return ex;
}


PYBIND11_MODULE(_component, m)
{
	m.def("get_component", get_component);
}
