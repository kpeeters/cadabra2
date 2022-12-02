#include <algorithm>
#include <iostream>
#include <limits>
#include <numeric>
#include "Adjform.hh"
#include "Cleanup.hh"
#include "Compare.hh"
#include "properties/IndexInherit.hh"
#include "properties/Symbol.hh"
#include "properties/Coordinate.hh"
#include "properties/Trace.hh"

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

	bool is_coordinate(const Kernel& kernel, Ex::iterator it)
	{
		if (it->is_index()) {
			auto coord = kernel.properties.get<Coordinate>(it, true);
			auto integer = it->is_integer();
			return coord != nullptr || integer;
		}
		return false;
	}

	bool is_index(const Kernel& kernel, Ex::iterator it, bool include_coordinates)
	{
		if (it->is_index()) {
			// Ignore things defined with the symbol property, and rational numbers
			auto symbol = kernel.properties.get<Symbol>(it, true);
			auto rational = it->is_rational() && !it->is_integer();
			return
				symbol == nullptr &&
				!rational &&
				(include_coordinates || !is_coordinate(kernel, it));
		}
		return false;
	}

	Adjform::Adjform()
	{

	}

	Adjform::const_iterator Adjform::begin() const
	{
		return data.cbegin();
	}

	Adjform::const_iterator Adjform::end() const
	{
		return data.cend();
	}

	Adjform::size_type Adjform::index_of(value_type index, size_type offset) const
	{
		auto pos = std::find(begin() + offset, end(), index);
		return std::distance(begin(), pos);
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


	bool Adjform::is_free_index(Adjform::size_type pos) const
	{
		return data[pos] < 0;
	}

	bool Adjform::is_dummy_index(Adjform::size_type pos) const
	{
		return data[pos] >= 0;
	}

	Adjform::size_type Adjform::n_free_indices() const
	{
		return std::count_if(data.begin(), data.end(), [](value_type idx) { return idx < 0; });
	}

	Adjform::size_type Adjform::n_dummy_indices() const
	{
		return size() - n_free_indices();
	}

	bool Adjform::resolve_dummy(value_type value)
	{
		// Find positions of both indices
		size_type posA = index_of(value);
		if (posA == size())
			return false;
		size_type posB = index_of(value, posA + 1);
		if (posB == size())
			return false;

		// Contract
		data[posA] = posB;
		data[posB] = posA;
		return true;
	}

	void Adjform::push_index(value_type value)
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

	void Adjform::push_indices(const Adjform& other)
	{
		size_t start_size = size();
		for (auto index : other) {
			if (index > 0)
				push_coordinate(index + start_size);
			else
				push_index(index);
		}
	}

	void Adjform::push_coordinate(value_type value)
	{
		data.push_back(value);
	}

	void Adjform::push_coordinates(const Adjform& other)
	{
		size_t start_size = size();
		for (auto index : other) {
			if (index > 0)
				push_coordinate(index + start_size);
			else
				push_coordinate(index);
		}
	}

	void Adjform::push(Ex::iterator it, IndexMap& index_map, const Kernel& kernel)
	{
		auto val = index_map.get_free_index(it);
		if (IndexMap::is_coordinate(kernel, it))
			push_coordinate(val);
		else
			push_index(val);
	}

	void Adjform::swap(size_type a, size_type b)
	{
		// do nothing if they point to each other
		if (data[a] == b && data[b] == a)
			return;

		// update pointed-to positions
		if (is_dummy_index(a))
			data[data[a]] = b;
		if (is_dummy_index(b))
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

	void Adjform::sort()
	{
		std::sort(data.begin(), data.end());
		auto dummy_start = std::find_if(data.begin(), data.end(), [](value_type v) { return v >= 0; });
		for (size_t pos = std::distance(data.begin(), dummy_start); pos < data.size(); pos += 2) {
			data[pos] = pos + 1;
			data[pos + 1] = pos;
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
				assert(counts.size() < std::numeric_limits<size_type>::max());
				if((size_type)counts.size() <= perm[i])
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
					assert(elem > 0);
					if((size_t)elem > i)
						--elem;
				}
				counts.erase(counts.begin() + i);
				--i;
			}
		}

		size_t perm_idx = 0;
		for (size_t i = 0; i < perm.size() - 1; ++i) {
			size_t num = ifactorial(perm.size() - i - 1);
			for (size_type j = 0; j < perm[i]; ++j) {
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
			else if ((size_t)(data[i]) > i) {
				res[i] = 'a' + next_free_index;
				++next_free_index;
			}
			else {
				res[i] = res[data[i]];
			}
		}
		return res;
	}


	IndexMap::IndexMap(const Kernel& kernel)
		: comp(std::make_unique<Ex_comparator>(kernel.properties))
		, data(std::make_unique<Ex>("T"))
	{

	}

	IndexMap::~IndexMap()
	{
		
	}

	Adjform::value_type IndexMap::get_free_index(Ex::iterator index)
	{
		Adjform::value_type i = 0;
		Ex::iterator head = data->begin();
		for (Ex::sibling_iterator beg = head.begin(), end = head.end(); beg != end; ++beg) {
			comp->clear();
			auto res = comp->equal_subtree(index, beg, Ex_comparator::useprops_t::never, true);
			if (res == Ex_comparator::match_t::subtree_match)
				return -(i + 1);
			++i;
		}
		data->append_child(head, index);
		return -(Adjform::value_type)data->begin().number_of_children();
	}

	bool IndexMap::is_coordinate(const Kernel& kernel, Ex::iterator index)
	{
		if (index->is_integer())
			return true;
		auto symb = kernel.properties.get<Symbol>(index, true);
		if (symb)
			return true;
		auto coord = kernel.properties.get<Coordinate>(index, true);
		if (coord)
			return true;
		return false;
	}

	ProjectedAdjform::integer_type ProjectedAdjform::zero = 0;

	ProjectedAdjform::ProjectedAdjform()
	{

	}

	ProjectedAdjform::ProjectedAdjform(const Adjform& adjform, const ProjectedAdjform::integer_type& value)
	{
		set(adjform, value);
	}

	void ProjectedAdjform::combine(const ProjectedAdjform& other)
	{
		for (const auto& kv : other.data)
			add(kv.first, kv.second);
	}

	void ProjectedAdjform::combine(const ProjectedAdjform& other, integer_type factor)
	{
		for (const auto& kv : other.data)
			add(kv.first, kv.second * factor);
	}

	ProjectedAdjform& ProjectedAdjform::operator += (const ProjectedAdjform& other)
	{
		combine(other);
		return *this;
	}

	ProjectedAdjform operator + (ProjectedAdjform lhs, const ProjectedAdjform& rhs)
	{
		return lhs += rhs;
	}

	void ProjectedAdjform::multiply(const integer_type& k)
	{
		for (auto& kv : data)
			kv.second *= k;
	}

	ProjectedAdjform& ProjectedAdjform::operator *= (const integer_type& k)
	{
		multiply(k);
		return *this;
	}

	ProjectedAdjform operator * (ProjectedAdjform lhs, const ProjectedAdjform::integer_type& rhs)
	{
		lhs.multiply(rhs);
		return lhs;
	}

	ProjectedAdjform::iterator ProjectedAdjform::begin()
	{
		return data.begin();
	}

	ProjectedAdjform::const_iterator ProjectedAdjform::begin() const
	{
		return data.begin();
	}

	ProjectedAdjform::iterator ProjectedAdjform::end()
	{
		return data.end();
	}

	ProjectedAdjform::const_iterator ProjectedAdjform::end() const
	{
		return data.end();
	}

	void ProjectedAdjform::clear()
	{
		data.clear();
	}

	size_t ProjectedAdjform::size() const
	{
		return data.size();
	}

	size_t ProjectedAdjform::max_size() const
	{
		if (empty())
			return 0;
		return begin()->first.max_lehmer_code();
	}

	size_t ProjectedAdjform::n_indices() const
	{
		if (empty())
			return 0;
		return begin()->first.size();
	}

	bool ProjectedAdjform::empty() const
	{
		return data.empty();
	}

	const ProjectedAdjform::integer_type& ProjectedAdjform::get(const Adjform& adjform) const
	{
		auto pos = data.find(adjform);
		return (pos == data.end()) ? zero : pos->second;
	}

	void ProjectedAdjform::set(const Adjform& term, const ProjectedAdjform::integer_type& value)
	{
		if (!term.empty())
			set_(term, value);
	}

	void ProjectedAdjform::set_(const Adjform& term, const ProjectedAdjform::integer_type& value)
	{
		if (value != 0)
			data[term] = value;
		else
			data.erase(term);
	}

	void ProjectedAdjform::add(const Adjform& term, const ProjectedAdjform::integer_type& value)
	{
		if (!term.empty())
			add_(term, value);
	}

	void ProjectedAdjform::add_(const Adjform& term, const ProjectedAdjform::integer_type& value)
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

	void ProjectedAdjform::apply_young_symmetry(const std::vector<size_t>& indices, bool antisymmetric)
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
					size_t loc = positions[rk] + directions[rk];
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
				for (size_t i = r + 1; i < directions.size(); ++i)
					directions[i] = -directions[i];
			}
		}
	}

	void ProjectedAdjform::apply_ident_symmetry(const std::vector<size_t>& positions, size_t n_indices)
	{
		apply_ident_symmetry(positions, n_indices, std::vector<std::vector<int>>(positions.size(), std::vector<int>(positions.size(), 1)));
	}

	void ProjectedAdjform::apply_ident_symmetry(const std::vector<size_t>& positions, size_t n_indices, const std::vector<std::vector<int>>& commutation_matrix)
	{
		for (size_t i = 0; i < positions.size() - 1; ++i) {
			auto old_data = data;
			for (size_t j = i + 1; j < positions.size(); ++j) {
				int sign = commutation_matrix[i][j];
				if (sign != 0) {
					for (const auto& kv : old_data) {
						auto term = kv.first;
						for (size_t k = 0; k < n_indices; ++k)
							term.swap(positions[i] + k, positions[j] + k);
						add_(term, kv.second * sign);
					}
				}
			}
		}
	}

	void ProjectedAdjform::apply_cyclic_symmetry()
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

std::ostream& operator << (std::ostream& os, const cadabra::ProjectedAdjform& adjex)
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

