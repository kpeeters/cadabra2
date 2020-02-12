#include <algorithm>
#include <iostream>
#include "Adjform.hh"

// Get the next permutation of term and return the number of swaps
// required for the transformation
int next_perm(std::vector<size_t>& term)
{
	int n = term.size();

	// Find longest non-increasing suffix to get pivot
	int pivot = n - 2;
	while (pivot > -1) {
		if (term[pivot + 1] > term[pivot])
			break;
		--pivot;
	}

	// Entire sequence is already sorted, return
	if (pivot == -1)
		return 0;

	// Find rightmost element greater than pivot
	int idx = n - 1;
	while (idx > pivot) {
		if (term[idx] > term[pivot])
			break;
		--idx;
	}

	// Swap with pivot
	std::swap(term[pivot], term[idx]);

	// Reverse the suffix
	int swaps = 1;
	int maxswaps = (n - pivot - 1) / 2;
	for (int i = 0; i < maxswaps; ++i) {
		if (term[pivot + i + 1] != term[n - i - 1]) {
			std::swap(term[pivot + i + 1], term[n - i - 1]);
			++swaps;
		}
	}

	return swaps;
}

// Returns the position of 'val' between 'begin' and 'end', starting
// the search at 'offset'
template <typename It, typename T>
size_t index_of(It begin, It end, const T& val, size_t offset = 0)
{
	auto pos = std::find(begin + offset, end, val);
	return std::distance(begin, pos);
}

cadabra::Adjform collapse_dummy_indices(cadabra::Adjform term)
{
	cadabra::AdjformIdx next_free_index = term.size();
	for (size_t i = 0; i < term.size(); ++i) {
		if (term[i] >= 0 && term[i] < (cadabra::AdjformIdx)term.size()) {
			term[term[i]] = next_free_index;
			term[i] = next_free_index;
			++next_free_index;
		}
	}
	return term;
}

cadabra::Adjform expand_dummy_indices(cadabra::Adjform term)
{
	for (size_t idx = 0; idx < term.size(); ++idx) {
		if (term[idx] >= (cadabra::AdjformIdx)term.size()) {
			auto pos = index_of(term.begin(), term.end(), term[idx], idx + 1);
			term[idx] = pos;
			term[pos] = idx;
		}
	}
	return term;
}

namespace cadabra {

	Adjform IndexMap::to_adjform(Ex::iterator it)
	{
		Adjform adjform;
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

	AdjformIdx IndexMap::get_free_index(nset_t::iterator name)
	{
		auto pos = std::find(data.begin(), data.end(), name);
		if (pos == data.end()) {
			data.push_back(name);
			return -(AdjformIdx)data.size();
		}
		else {
			return -(AdjformIdx)std::distance(data.begin(), pos) - 1;
		}
	}

	AdjformEx::AdjformEx()
	{

	}

	AdjformEx::AdjformEx(const Adjform& adjform, mpq_class value)
	{
		set(adjform, value);
	}

	AdjformEx::AdjformEx(Ex::iterator it, IndexMap& index_map)
	{
		set(index_map.to_adjform(it), *it->multiplier);
	}

	mpq_class AdjformEx::compare(const AdjformEx& other) const
	{
		// Early failure checks
		if (data.empty() || data.size() != other.data.size())
			return 0;

		// Find the numeric factor between the first two terms, then loop over all
		// other terms checking that the factor is the same. If not, return 0
		auto a_it = data.begin(), b_it = other.data.begin(), a_end = data.end();
		mpq_class factor = a_it->second / b_it->second;
		while (a_it != a_end) {
			if (a_it->first != b_it->first)
				return 0;
			if (a_it->second / b_it->second != factor)
				return 0;
			++a_it, ++b_it;
		}
		return factor;
	}

	void AdjformEx::combine(const AdjformEx& other)
	{
		for (const auto& kv : other.data)
			add(kv.first, kv.second);
	}

	void AdjformEx::combine(const AdjformEx& other, mpq_class factor)
	{
		for (const auto& kv : other.data) 
			add(kv.first, kv.second * factor);
	}

	void AdjformEx::multiply(mpq_class k)
	{
		for (auto& kv : data)
			kv.second *= k;
	}

	AdjformEx::iterator AdjformEx::begin()
	{
		return data.begin();
	}

	AdjformEx::const_iterator AdjformEx::begin() const
	{
		return data.begin();
	}

	AdjformEx::iterator AdjformEx::end()
	{
		return data.end();
	}

	AdjformEx::const_iterator AdjformEx::end() const
	{
		return data.end();
	}



	void AdjformEx::clear()
	{
		data.clear();
	}

	size_t AdjformEx::size() const
	{
		return data.size();
	}

	bool AdjformEx::empty() const
	{
		return data.empty();
	}

	void AdjformEx::set(Adjform term, mpq_class value)
	{
		if (!term.empty())
			set_(term, value);
	}

	void AdjformEx::set_(Adjform term, mpq_class value)
	{
		if (value != 0)
			data[term] = value;
		else
			data.erase(term);
	}

	void AdjformEx::add(Adjform term, mpq_class value)
	{
		if (!term.empty()) 
			add_(term, value);
	}

	void AdjformEx::add_(Adjform term, mpq_class value)
	{
		auto elem = data.find(term);

		if (elem == data.end() && value != 0) {
			data[term] = value;
		}
		else {
			elem->second += value;
			if (elem->second == 0)
				data.erase(elem);
		}
	}

	void AdjformEx::apply_young_symmetry(const std::vector<size_t>& indices, bool antisymmetric)
	{
		map_t old_data;
		std::swap(old_data, data);

		// Loop over all entries, for each one looping over all permutations
		// of the indices to be symmetrized and creating a new term for that
		// permutation; then add the new term to the list of entries
		for (const auto& kv : old_data) {
			auto perm = indices;
			int parity = 1;
			int swaps = 2;
			do {
				if (antisymmetric && swaps % 2 != 0)
					parity *= -1;
				auto ret = kv.first;
				for (size_t i = 0; i < indices.size(); ++i) {
					ret[indices[i]] = kv.first[perm[i]];
				}
				for (auto index : indices) {
					if (ret[index] >= 0)
						ret[ret[index]] = index;
				}
				add_(ret, parity * kv.second);
			} while ((swaps = next_perm(perm)));
		}
	}

	void AdjformEx::apply_ident_symmetry(std::vector<size_t> positions, size_t n_indices)
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
				Adjform out = term;
				for (size_t i = 0; i < perm.size(); ++i) {
					for (size_t k = 0; k < n_indices; ++k) {
						out[perm[i] + k] = term[positions[i] + k];
					}
				}
				add_(expand_dummy_indices(out), kv.second);
			}
		}
	}

	void AdjformEx::apply_cyclic_symmetry()
	{
		if (data.empty())
			return;

		size_t n_indices = data.begin()->first.size();
		size_t n_steps = n_indices - 1;
		map_t old_data = data;

		for (const auto& kv : old_data) {
			auto perm = kv.first;
			for (size_t step = 0; step < n_steps; ++step) {
				for (auto& idx : perm) {
					if (idx >= 0)
						++idx;
					if (idx == (AdjformIdx)n_indices)
						idx = 0;
				}
				std::rotate(perm.begin(), perm.begin() + perm.size() - 1, perm.end());
				add_(perm, kv.second);
			}
		}
	}

}

std::ostream& operator << (std::ostream& os, const cadabra::Adjform& adjform)
{
	for (const auto& idx : adjform)
		os << idx << ' ';
	return os;
}

std::ostream& operator << (std::ostream& os, const cadabra::AdjformEx& adjex)
{
	size_t i = 0;
	size_t max = std::min(std::size_t(200), adjex.size());
	auto it = adjex.begin();
	while (i < max) {
		os << it->first << '\t' << it->second << '\n';
		++i;
		++it;
	}
	if (max < adjex.size()) {
		os << "(skipped " << (adjex.size() - max) << " terms)\n";
	}
	return os;
}
