#pragma once

#include "Storage.hh"

namespace cadabra
{
	enum HashFlags : unsigned int
	{
		HASH_DEFAULT = 0x0,
		HASH_IGNORE_TOP_MULTIPLIER = 0x1,
		HASH_IGNORE_MULTIPLIER = 0x2,
		HASH_IGNORE_PRODUCT_ORDER = 0x4,
		HASH_IGNORE_SUM_ORDER = 0x8,
		HASH_IGNORE_INDEX_ORDER = 0x10,
		HASH_IGNORE_PARENT_REL = 0x20,
		HASH_IGNORE_BRACKET_TYPE = 0x40,
		HASH_IGNORE_CHILDREN = 0x80,
		HASH_IGNORE_NAMES = 0x100,
		HASH_IGNORE_INDICES = 0x200,
		HASH_IGNORE_CHILD_ORDER = 0x400
	};

	class Ex_hasher
	{
	public:
		using result_t = size_t;
		Ex_hasher();
		Ex_hasher(HashFlags flags);

		result_t operator () (const Ex& ex) const;
		result_t operator () (Ex::iterator it) const;

		void set_flags(HashFlags flags);
		HashFlags get_flags() const;
		void add_flags(HashFlags flags);
		void remove_flags(HashFlags flags);

	private:
		result_t hash(Ex::iterator it, bool toplevel) const;
		bool flag_set(HashFlags flag) const;

		HashFlags flags;
	};

	HashFlags operator ~ (HashFlags flags);
	HashFlags operator | (HashFlags lhs, HashFlags rhs);
	HashFlags operator & (HashFlags lhs, HashFlags rhs);
	HashFlags& operator |= (HashFlags& lhs, HashFlags rhs);
	HashFlags& operator &= (HashFlags& lhs, HashFlags rhs);

	Ex_hasher::result_t hash_ex(Ex::iterator it, HashFlags flags = HASH_DEFAULT);
	bool hash_compare(Ex::iterator lhs, Ex::iterator rhs, HashFlags flags = HASH_DEFAULT);
}
