#include <vector>
#include "properties/Coordinate.hh"
#include "algorithms/expand_dummies.hh"
#include "Cleanup.hh"
#include "Compare.hh"
#include "IndexClassifier.hh"
#include "IndexIterator.hh"
#include "Functional.hh"
#include "substitute.hh"

using namespace cadabra;

expand_dummies::expand_dummies(const Kernel& kernel, Ex& ex, const Ex* components, bool zero_missing_components)
	: Algorithm(kernel, ex)
	, comp(kernel.properties)
	, components(components)
	, zero_missing_components(zero_missing_components)
	{
	enumerate_patterns();
	}

void expand_dummies::enumerate_patterns()
{
	// If components is provided create a list of patterns which are inside
	// the components 
	if (components != nullptr) {
		do_list(*components, components->begin(), [this] (Ex::iterator c) {
			char idx_placeholder = 'A';
			Ex pattern(c.begin());
			auto beg = index_iterator::begin(kernel.properties, pattern.begin());
			auto end = index_iterator::end(kernel.properties, pattern.begin());
			while (beg != end) {
				auto pr = beg->fl.parent_rel;
				auto repl = pattern.replace(beg, str_node(std::string(1, idx_placeholder++) + "?"));
				repl->fl.parent_rel = pr;
				beg.walk = repl;
				beg.node = repl.node;
				++beg;
				}

			bool found = false;
			for (const auto& other : component_patterns) {
				comp.clear();
				if (comp.equal_subtree(pattern.begin(), other.begin()) == Ex_comparator::match_t::subtree_match) {
					found = true;
					break;
				}
			}
			if (!found)
				component_patterns.push_back(pattern);
			return true;
			});
		}
}

bool expand_dummies::can_apply(iterator it)
	{
	if (*it->name == "\\sum" || *it->name == "\\equals")
		return false;

	// Require a node which has dummy indices with values attached
	std::vector<Ex::iterator> candidates;
	auto beg = index_iterator::begin(kernel.properties, it);
	auto end = index_iterator::end(kernel.properties, it);
	while (beg != end) {
		auto prop = kernel.properties.get<Indices>(beg);
		if (prop && !prop->values.empty()) {
			for (const auto& candidate : candidates) {
				comp.clear();
				auto res = comp.equal_subtree(candidate, beg, Ex_comparator::useprops_t::always, true);
				if (res == Ex_comparator::match_t::subtree_match)
					return true;
				}
				candidates.push_back(beg);
			}
		++beg;
		}
	 return false;
	 }

Algorithm::result_t expand_dummies::apply(iterator& it)
	{
	// Create a new expression which we will modify and a sum node which will
	// replace it
	Ex pat(it), sum("\\sum");
	std::vector<std::vector<iterator>> dummies;
	std::vector<const std::vector<Ex>*> values;

	index_map_t full_ind_free, full_ind_dummy;
	classify_indices(pat.begin(), full_ind_free, full_ind_dummy);
	for (const auto& kv : full_ind_dummy) {
		auto pos = std::find_if(dummies.begin(), dummies.end(),
			[this, kv](const std::vector<iterator>& lhs) {
			comp.clear();
			auto res = comp.equal_subtree(lhs[0], kv.second, Ex_comparator::useprops_t::always, true);
			return res == Ex_comparator::match_t::subtree_match;
		});
		if (pos == dummies.end()) {
			auto prop = kernel.properties.get<Indices>(kv.first.begin(), true);
			if (prop && !prop->values.empty()) {
				dummies.emplace_back(1, kv.second);
				values.push_back(&(prop->values));
			}
		}
		else {
			pos->push_back(kv.second);
		}
	}
	 
	// Set up 'positions' to hold iterators into the corresponding elements of the
	// 'values' vector, we will loop through all possible combinations
	std::vector<std::vector<Ex>::const_iterator> positions;
	for (const auto& vec : values)
		positions.push_back(vec->begin());

	do {
		// Rewrite each dummy pair with the coordinate pointed to by the
		// positions vector and append to the sum
		for (size_t i = 0; i < dummies.size(); ++i) {
			for (iterator& dummy : dummies[i])
				dummy = pat.replace_index(dummy, positions[i]->begin(), true);
		}

		auto term = sum.append_child(sum.begin(), pat.begin());
		fill_components(term);

		// Increment the positions vector
		for (size_t i = 0; i < positions.size(); ++i) {
			++positions[i];
			if (positions[i] != values[i]->end())
				break;
			else if (i != positions.size() - 1)
				positions[i] = values[i]->begin();
			}
		} while (positions.back() != values.back()->end());
	 
	// Replace the node with the new sum
	it = tr.replace(it, sum.begin());

	return result_t::l_applied;
	}

void expand_dummies::fill_components(Ex::iterator it)
	{
	if (components == nullptr)
		return;

	Ex::post_order_iterator walk = it, last = it;
	walk.descend_all();
	++last;

	do {
		auto next = walk;
		++next;

		// Try to match a pattern against the current node
		for (const auto& pattern : component_patterns) {
			comp.clear();
			auto res = comp.equal_subtree(pattern.begin(), walk);
			if ((res != Ex_comparator::match_t::subtree_match) &&
				 (res != Ex_comparator::match_t::match_index_greater) &&
				 (res != Ex_comparator::match_t::match_index_less))
				continue;

			// Ensure all indices are coordinates
			auto ibeg = index_iterator::begin(kernel.properties, walk);
			auto iend = index_iterator::end(kernel.properties, walk);
			while (ibeg != iend) {
				if (kernel.properties.get<Coordinate>(ibeg, true) == nullptr)
					break;
				++ibeg;
				}
			if (ibeg != iend)
				continue;

			// Find the matching term in components
			bool replaced = false;
			auto head = components->begin();
			if (*head->name == "\\comma") {
				for (Ex::sibling_iterator cbeg = head.begin(), cend = head.end(); cbeg != cend; ++cbeg) {
					Ex::sibling_iterator term = cbeg.begin();
					comp.clear();
					if (comp.equal_subtree(term, walk) == Ex_comparator::match_t::subtree_match) {
						++term;
						if (walk == it)
							it = tr.replace(walk, term);
						else
							tr.replace(walk, term);
						replaced = true;
						break;
						}
					}
				}
			else {
				auto term = head.begin();
				comp.clear();
				if (comp.equal_subtree(walk, term) == Ex_comparator::match_t::subtree_match) {
					++term;
					if (walk == it)
						it = tr.replace(walk, term);
					else
						tr.replace(walk, term);
					replaced = true;
					}
				}
			if (!replaced && zero_missing_components) {
				// No rule found, term is 0
				tr.erase(it);
				return;
				}
			break;
			}
		walk = next;
		} while (walk != last);

		cleanup_dispatch(kernel, tr, it);
	}
