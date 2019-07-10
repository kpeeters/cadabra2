#include "Hash.hh"

// Based on boost's implementation of hash_combine
// <https://www.boost.org/doc/libs/1_54_0/doc/html/hash/reference.html#boost.hash_combine>
void hash_combine(size_t& seed, size_t modifier)
{
	seed ^= (modifier + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

template <typename T>
size_t do_hash(const T& t)
{
	std::hash<T> h;
	return h(t);
};

namespace cadabra
{
	Ex_hasher::Ex_hasher()
		: flags(HASH_DEFAULT)
	{

	}

	Ex_hasher::Ex_hasher(HashFlags flags)
		: flags(flags)
	{

	}

	Ex_hasher::result_t Ex_hasher::operator () (const Ex& ex) const
	{
		return hash(ex.begin(), true);
	}

	Ex_hasher::result_t Ex_hasher::operator () (Ex::iterator it) const
	{
		return hash(it, true);
	}

	Ex_hasher::result_t Ex_hasher::hash (Ex::iterator it, bool toplevel) const
	{
		size_t seed = 0;

		// Hash the underlying str_node
		if (!flag_set(HASH_IGNORE_NAMES))
			hash_combine(seed, do_hash(*it->name));
		if (!flag_set(HASH_IGNORE_MULTIPLIER) && !(toplevel && flag_set(HASH_IGNORE_TOP_MULTIPLIER)))
			hash_combine(seed, do_hash(it->multiplier->get_str()));
		// Offset the flags by different amounts to reduce collisions
		if (!flag_set(HASH_IGNORE_BRACKET_TYPE))
			hash_combine(seed, do_hash((it->fl.bracket + 1) << 4));
		if (!flag_set(HASH_IGNORE_PARENT_REL))
			hash_combine(seed, do_hash((it->fl.parent_rel + 1) << 8));

		if (!flag_set(HASH_IGNORE_CHILDREN) && it.number_of_children() > 0) {

			if (
				flag_set(HASH_IGNORE_CHILD_ORDER) ||
				(flag_set(HASH_IGNORE_SUM_ORDER) && *it->name == "\\sum") ||
				(flag_set(HASH_IGNORE_PRODUCT_ORDER) && *it->name == "\\prod")) {
				std::set<size_t> hashes;
				for (Ex::sibling_iterator beg = it.begin(), end = it.end(); beg != end; ++beg) {
					if (!flag_set(HASH_IGNORE_INDICES) && beg->is_index())
						hashes.insert(hash(beg, false));
				}
				for (size_t hash : hashes)
					hash_combine(seed, hash);
			}
			else {
				if (flag_set(HASH_IGNORE_INDICES)) {
					for (Ex::sibling_iterator beg = it.begin(), end = it.end(); beg != end; ++beg) {
						if (beg->is_index())
							continue;
						hash_combine(seed, hash(beg, false));
					}
				}
				else if (flag_set(HASH_IGNORE_INDEX_ORDER)) {
					std::set<size_t> hashes;
					for (Ex::sibling_iterator beg = it.begin(), end = it.end(); beg != end; ++beg) {
						if (beg->is_index())
							hashes.insert(hash(beg, false));
						else
							hash_combine(seed, hash(beg, false));
					}
					for (size_t hash : hashes)
						hash_combine(seed, hash);
				}
				else {
					for (Ex::sibling_iterator beg = it.begin(), end = it.end(); beg != end; ++beg) {
						hash_combine(seed, hash(beg, false));
					}
				}
			}
		}
		return seed;
	}

	bool Ex_hasher::flag_set(HashFlags flag) const
	{
		return (flags & flag);
	}

	using flags_base_t = std::underlying_type_t<HashFlags>;

	HashFlags operator ~ (HashFlags flags)
	{
		return (HashFlags)~(flags_base_t)(flags);
	}

	HashFlags operator | (HashFlags lhs, HashFlags rhs)
	{
		return (HashFlags)((flags_base_t)lhs | (flags_base_t)rhs);
	}

	HashFlags operator & (HashFlags lhs, HashFlags rhs)
	{
		return (HashFlags)((flags_base_t)lhs & (flags_base_t)rhs);
	}

	HashFlags& operator |= (HashFlags& lhs, HashFlags rhs)
	{
		return (HashFlags&)((flags_base_t&)lhs |= (flags_base_t)rhs);
	}

	HashFlags& operator &= (HashFlags& lhs, HashFlags rhs)
	{
		return (HashFlags&)((flags_base_t&)lhs &= (flags_base_t)rhs);
	}

	Ex_hasher::result_t hash_ex(Ex::iterator it, HashFlags flags)
	{
		Ex_hasher hasher(flags);
		return hasher(it);
	}

	bool hash_compare(Ex::iterator lhs, Ex::iterator rhs, HashFlags flags)
	{
		Ex_hasher hasher(flags);
		return hasher(lhs) == hasher(rhs);
	}
}
