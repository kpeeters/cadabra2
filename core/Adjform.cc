#include <algorithm>
#include <iostream>
#include <limits>
#include <numeric>
#include "Adjform.hh"
#include "Cleanup.hh"
#include "properties/Symbol.hh"
#include "properties/Coordinate.hh"

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

size_t slots_to_pairs(size_t slots)
{
	size_t res = 1;
	for (size_t i = 3; i < slots; i += 2)
		res *= i;
	return res;
}

size_t ifactorial(size_t n, size_t den = 1)
{
	if (n < 2)
		return 1;

	size_t res = 1;
	for (size_t k = den + 1; k <= n; ++k)
		res *= k;
	return res;
}

namespace cadabra {
	bool is_index(const Kernel& kernel, Ex::iterator it)
	{
		if (it->is_index()) {
			auto s = kernel.properties.get<Symbol>(it);
			auto c = kernel.properties.get<Coordinate>(it);
			return !it->is_integer() && !s && !c;
		}
		return false;
	}

	bool has_indices(const cadabra::Kernel& kernel, cadabra::Ex::iterator it)
	{
		using namespace cadabra;
		for (Ex::iterator beg = it.begin(), end = it.end(); beg != end; ++beg) {
			if (is_index(kernel, beg))
				return true;
		}
		return false;
	}

	Adjform::Adjform()
	{

	}

	Adjform::Adjform(Ex::iterator it, IndexMap& index_map, const Kernel& kernel)
	{
		Ex_hasher hasher(HashFlags::HASH_IGNORE_PARENT_REL);
		for (Ex::iterator beg = it.begin(), end = it.end(); beg != end; ++beg) {
			if (is_index(kernel, beg)) {
				beg.skip_children();
				push_back(index_map.get_free_index(hasher(beg)));
			}
		}
	}

	Adjform::const_iterator Adjform::begin() const
	{
		return data.cbegin();
	}

	Adjform::const_iterator Adjform::end() const
	{
		return data.cend();
	}

	bool Adjform::operator < (const Adjform& other) const
	{
		return data < other.data;
	}

	bool Adjform::operator == (const Adjform& other) const
	{
		return data == other.data;
	}

	bool Adjform::operator != (const Adjform& other) const
	{
		return data != other.data;
	}

	Adjform::const_reference Adjform::operator [] (Adjform::size_type idx) const
	{
		return data[idx];
	}

	Adjform::size_type Adjform::size() const
	{
		return (size_type)data.size();
	}

	Adjform::size_type Adjform::max_size() const
	{
		return std::numeric_limits<value_type>::max();
	}

	bool Adjform::empty() const
	{
		return data.empty();
	}

	Adjform::size_type Adjform::n_free_indices() const
	{
		return std::count_if(data.begin(), data.end(), is_free_index);
	}

	Adjform::size_type Adjform::n_dummy_indices() const
	{
		return std::count_if(data.begin(), data.end(), is_dummy_index);
	}

	void Adjform::push_back(value_type value)
	{
		auto pos = std::find(data.begin(), data.end(), value);
		if (pos == data.end()) {
			data.push_back(value);
		}
		else {
			*pos = data.size();
			data.push_back(std::distance(data.begin(), pos));
		}
	}

	void Adjform::swap(size_type a, size_type b)
	{
		// do nothing if they point to each other
		if (data[a] == b && data[b] == a)
			return;

		// update pointed-to positions
		if (is_dummy_index(data[a]))
			data[data[a]] = b;
		if (is_dummy_index(data[b]))
			data[data[b]] = a;

		std::swap(data[a], data[b]);
	}

	void Adjform::rotate(size_type n)
	{
		if (size() < 2)
			return;
		n = (n % size() + size()) % size();

		std::rotate(data.begin(), data.end() - n, data.end());
		for (auto& idx : data) {
			if (idx >= 0)
				idx = (idx + n) % data.size();
		}
	}

	uint64_t Adjform::to_lehmer_code() const
	{
		std::vector<size_t> counts = { 0 };
		uint64_t dummy_idx = 0;
		size_t n_dummies = n_dummy_indices();
		size_t remaining_dummies = n_dummies;
		array_type perm(size());

		for (value_type i = 0; i < size(); ++i) {
			if (data[i] < 0) {
				perm[i] = -data[i];
				if (counts.size() <= perm[i])
					counts.resize(perm[i] + 1, 0);
				++counts[perm[i]];
			}
			else {
				if (data[i] > i) {
					size_t dist = 0;
					for (value_type j = i + 1; j < size(); ++j) {
						if (data[j] == i) {
							remaining_dummies -= 2;
							dummy_idx += dist * slots_to_pairs(remaining_dummies);
						}
						else if (data[j] > i) {
							dist += 1;
						}
					}
				}
				perm[i] = 0;
				++counts[0];
			}
		}

		for (size_t i = 0; i < counts.size(); ++i) {
			if (counts[i] == 0) {
				for (auto& elem : perm) {
					if (elem > i)
						--elem;
				}
				counts.erase(counts.begin() + i);
				--i;
			}
		}

		size_t perm_idx = 0;
		for (size_t i = 0; i < perm.size() - 1; ++i) {
			size_t num = ifactorial(perm.size() - i - 1);
			for (size_t j = 0; j < perm[i]; ++j) {
				if (counts[j] == 0)
					continue;
				--counts[j];
				size_t den = 1;
				for (size_t k = 0; k < counts.size(); ++k)
					den *= ifactorial(counts[k]);
				perm_idx += num / den;
				++counts[j];
			}
			--counts[perm[i]];
		}
		return perm_idx * slots_to_pairs(n_dummies) + dummy_idx;
	}

	uint64_t Adjform::max_lehmer_code() const
	{
		auto dummies = n_dummy_indices();
		uint64_t res = ifactorial(data.size(), dummies);
		res *= slots_to_pairs(dummies);
		return res;
	}

	std::string Adjform::to_string() const
	{
		std::string res(data.size(), ' ');
		size_type next_free_index = size();
		for (size_t i = 0; i < data.size(); ++i) {
			if (data[i] < 0) {
				res[i] = 'a' - data[i] - 1;
			}
			else if (data[i] > i) {
				res[i] = 'a' + next_free_index;
				++next_free_index;
			}
			else {
				res[i] = res[data[i]];
			}
		}
		return res;
	}

	bool is_free_index(Adjform::value_type idx)
	{
		return idx < 0;
	}

	bool is_dummy_index(Adjform::value_type idx)
	{
		return idx >= 0;
	}

	Adjform::value_type IndexMap::get_free_index(Ex_hasher::result_t index)
	{
		auto pos = std::find(data.begin(), data.end(), index);
		if (pos == data.end()) {
			data.push_back(index);
			return -(Adjform::value_type)data.size();
		}
		else {
			return -(Adjform::value_type)std::distance(data.begin(), pos) - 1;
		}
	}

	AdjformEx::rational_type AdjformEx::zero = 0;

	AdjformEx::AdjformEx()
	{

	}

	AdjformEx::AdjformEx(const Adjform& adjform, const AdjformEx::rational_type& value, const Ex& prefactor)
		: prefactor(prefactor)
	{
		set(adjform, value);
	}

	AdjformEx::AdjformEx(const Adjform& adjform, const rational_type& value, Ex::iterator prefactor)
		: prefactor(prefactor)
	{
		set(adjform, value);
	}

	AdjformEx::AdjformEx(Ex::iterator it, IndexMap& index_map, const Kernel& kernel)
		: prefactor(str_node("\\prod"))
		, tensor(str_node("\\prod"))
	{
		set(Adjform(it, index_map, kernel));

		if (*it->name == "\\prod") {
			for (Ex::sibling_iterator beg = it.begin(), end = it.end(); beg != end; ++beg) {
				if (has_indices(kernel, beg))
					tensor.append_child(tensor.begin(), (Ex::iterator)beg);
				else
					prefactor.append_child(prefactor.begin(), (Ex::iterator)beg);
			}
			cadabra::multiply(prefactor.begin()->multiplier, *it->multiplier);
		}
		else {
			if (has_indices(kernel, it)) {
				tensor.append_child(tensor.begin(), (Ex::iterator)it);
				cadabra::multiply(prefactor.begin()->multiplier, *it->multiplier);
			}
			else {
				prefactor.append_child(prefactor.begin(), (Ex::iterator)it);
			}
		}

		Ex::iterator prefactor_begin = prefactor.begin();
		cleanup_dispatch(kernel, prefactor, prefactor_begin);
		Ex::iterator tensor_begin = tensor.begin();
		cleanup_dispatch(kernel, tensor, tensor_begin);
		one(tensor_begin->multiplier);
	}

	AdjformEx::rational_type AdjformEx::compare(const AdjformEx& other) const
	{
		// Early failure checks
		if (data.empty() || data.size() != other.data.size())
			return 0;

		// Find the numeric factor between the first two terms, then loop over all
		// other terms checking that the factor is the same. If not, return 0
		auto a_it = data.begin(), b_it = other.data.begin(), a_end = data.end();
		rational_type factor = a_it->second / b_it->second;
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

	void AdjformEx::combine(const AdjformEx& other, AdjformEx::rational_type factor)
	{
		for (const auto& kv : other.data) 
			add(kv.first, kv.second * factor);
	}

	void AdjformEx::multiply(const AdjformEx::rational_type& k)
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

	size_t AdjformEx::max_size() const
	{
		if (empty())
			return 0;
		return begin()->first.max_lehmer_code();
	}

	size_t AdjformEx::n_indices() const
	{
		if (empty())
			return 0;
		return begin()->first.size();
	}

	bool AdjformEx::empty() const
	{
		return data.empty();
	}

	const Ex& AdjformEx::get_prefactor_ex() const
	{
		return prefactor;
	}

	Ex& AdjformEx::get_prefactor_ex()
	{
		return prefactor;
	}

	const Ex& AdjformEx::get_tensor_ex() const
	{
		return tensor;
	}

	Ex& AdjformEx::get_tensor_ex()
	{
		return tensor;
	}

	const AdjformEx::rational_type& AdjformEx::get(const Adjform& adjform) const
	{
		auto pos = data.find(adjform);
		return (pos == data.end()) ? zero : pos->second;
	}

	void AdjformEx::set(const Adjform& term, const AdjformEx::rational_type& value)
	{
		if (!term.empty())
			set_(term, value);
	}

	void AdjformEx::set_(const Adjform& term, const AdjformEx::rational_type& value)
	{
		if (value != 0)
			data[term] = value;
		else
			data.erase(term);
	}

	void AdjformEx::add(const Adjform& term, const AdjformEx::rational_type& value)
	{
		if (!term.empty()) 
			add_(term, value);
	}

	void AdjformEx::add_(const Adjform& term, const AdjformEx::rational_type& value)
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
		map_t old_data = data;
		
		for (const auto& kv : old_data) {
			std::vector<int> values(indices.size());
			std::iota(values.begin(), values.end(), 1);
			std::vector<int> positions(indices.size() + 1);
			std::iota(positions.begin(), positions.end(), -1);
			std::vector<int> directions(indices.size() + 1, -1);
			int sign = -1;
			auto term = kv.first;
			while (true) {
				int r = 0;
				for (int rk = values.size(); rk > 0; --rk) {
					int loc = positions[rk] + directions[rk];
					if (loc >= 0 && loc < values.size() && values[loc] < rk) {
						r = rk;
						break;
					}
				}
				if (r == 0)
					break;

				int r_loc = positions[r];
				int l_loc = r_loc + directions[r];
				int l = values[l_loc];

				term.swap(indices[values[l_loc] - 1], indices[values[r_loc] - 1]);
				add_(term, kv.second * (antisymmetric ? sign : 1));

				std::swap(values[l_loc], values[r_loc]);
				std::swap(positions[l], positions[r]);
				sign *= -1;
				for (int i = r + 1; i < directions.size(); ++i)
					directions[i] = -directions[i];
			}
		}
	}

	void AdjformEx::apply_ident_symmetry(std::vector<size_t> positions, size_t n_indices)
	{
		for (size_t i = 0; i < positions.size() - 1; ++i) {
			auto old_data = data;
			for (size_t j = i + 1; j < positions.size(); ++j) {
				for (const auto& kv : old_data) {
					auto term = kv.first;
					for (size_t k = 0; k < n_indices; ++k) 
						term.swap(positions[i] + k, positions[j] + k);
					add_(term, kv.second);
				}
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
				perm.rotate(1);
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

