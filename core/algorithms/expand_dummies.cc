#include "expand_dummies.hh"
#include "Compare.hh"
#include "IndexIterator.hh"
#include "Functional.hh"
#include "substitute.hh"
#include "properties/Coordinate.hh"
#include <vector>


using namespace cadabra;


expand_dummies::expand_dummies(const Kernel& kernel, Ex& ex, const Ex* components)
    : Algorithm(kernel, ex)
    , comp(kernel.properties)
    , components(components)
    {
        enumerate_patterns();
    }

void expand_dummies::enumerate_patterns()
{
        // If components is provided create a list of patterns which are inside
        // the components 
        if (components != nullptr) {
            Ex tmp("#");
            do_list(*components, components->begin(), [this, &tmp] (Ex::iterator c) {
                Ex pattern(c.begin());
                auto beg = index_iterator::begin(kernel.properties, pattern.begin());
                auto end = index_iterator::end(kernel.properties, pattern.end());
                while (beg != end) {
                    auto next = ++beg;
                    pattern.replace_index(beg, tmp.begin(), true);
                    beg = next;
                }
                component_patterns.insert(pattern);
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
    std::vector<Ex::iterator> candidates;
    std::vector<std::pair<Ex::iterator, Ex::iterator>> dummy_pairs;
    std::vector<const std::vector<Ex>*> values;

    // Obtain iterators to the dummies and hold a parallel vector with a pointer
    // to the the associated 'values' vectors
    auto beg = index_iterator::begin(kernel.properties, pat.begin());
    auto end = index_iterator::end(kernel.properties, pat.begin());
    while (beg != end) {
        auto prop = kernel.properties.get<Indices>(beg);
        if (prop && !prop->values.empty()) {
            bool matched = false;
            for (auto c1 = candidates.begin(), c2 = candidates.end(); c1 != c2; ++c1) {
                comp.clear();
                auto res = comp.equal_subtree(*c1, beg, Ex_comparator::useprops_t::always, true);
                if (res == Ex_comparator::match_t::subtree_match) {
                    dummy_pairs.emplace_back(*c1, beg);
                    values.push_back(&(prop->values));
                    candidates.erase(c1);
                    break;
                    }
                }
            if (!matched)
                candidates.push_back(beg);
            }
        ++beg;
       }
    
    // Set up 'positions' to hold iterators into the corresponding elements of the
    // 'values' vector, we will loop through all possible combinations
    std::vector<std::vector<Ex>::const_iterator> positions;
    for (const auto& vec : values)
        positions.push_back(vec->begin());

    do {
        // Rewrite each dummy pair with the coordinate pointed to by the
        // positions vector and append to the sum
        for (size_t i = 0; i < dummy_pairs.size(); ++i) {
            dummy_pairs[i].first = pat.replace_index(dummy_pairs[i].first, positions[i]->begin(), true);
            dummy_pairs[i].second = pat.replace_index(dummy_pairs[i].second, positions[i]->begin(), true);
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

    Ex::iterator beg = it, end = it;
    ++end;

    while (beg != end) {
        // Try to match a pattern against the current node
        for (const auto& pattern : component_patterns) {
            comp.clear();
            auto res = comp.equal_subtree(pattern.begin(), beg);
            if (res != Ex_comparator::match_t::subtree_match)
                continue;

            // Ensure all indices are coordinates
            auto ibeg = index_iterator::begin(kernel.properties, beg);
            auto iend = index_iterator::end(kernel.properties, beg);
            while (ibeg != iend) {
                if (kernel.properties.get<Coordinate>(ibeg) == nullptr)
                break;
            }
            if (ibeg != iend)
                continue;

            // Find the matching term in components
            bool replaced = false;
            if (*components->begin()->name == "\\comma") {
                auto head = components->begin();
                for (Ex::sibling_iterator cbeg = head.begin(), cend = head.end(); cbeg != cend; ++cbeg) {
                    Ex::sibling_iterator term = cbeg.begin();
                    comp.clear();
                    if (comp.equal_subtree(term, beg) == Ex_comparator::match_t::subtree_match) {
                        ++term;
                        beg = tr.replace(beg, term);
                        replaced = true;
                        break;
                        }
                    }
                }
            else {
                auto term = components->begin();
                comp.clear();
                if (comp.equal_subtree(beg, term) == Ex_comparator::match_t::subtree_match) {
                    ++term;
                    beg = tr.replace(beg, term);
                    replaced = true;
                    }

                }
            if (!replaced) {
                // No rule found, term is 0
                node_zero(beg);
                }
            break;
            }
        ++beg;
        }
    }
