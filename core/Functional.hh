
#pragma once

#include <functional>
#include "Storage.hh"

namespace cadabra {

	/// \ingroup core
	///
	/// Apply a function on every element of a list, or if the iterator
	/// 'it' does not point to a list, only on that single
	/// element. Handles lists wrapped in an \expression node as well.
	/// It is safe to remove the node pointed to by 'it' in 'f'.
	/// If your 'f' returns false, the loop is aborted immediately.

	void do_list(const Ex& tr, Ex::iterator it, std::function<bool(Ex::iterator)> f);

	/// \ingroup core
	///
	/// For lists as defined above for 'do_list', return their size (in case you
	/// really need to know the size before iterating over the elements).

	int list_size(const Ex& tr, Ex::iterator it);

	/// \ingroup core
	///
	/// Returns an iterator to the first element for which 'f' does not return tr.end().

	Ex::iterator find_in_list(const Ex& tr, Ex::iterator it, std::function<Ex::iterator(Ex::iterator)> f);

	/// \ingroup core
	///
	/// Returns an iterator to the first element for which 'f' returns 'true', or 'tr.end()'.

	Ex::iterator find_in_subtree(const Ex& tr, Ex::iterator it, std::function<bool(Ex::iterator)> f, bool including_head=true);

	/// \ingroup core
	///
	/// Ensure that the tree is a list, even if it contains only a single element.

	Ex make_list(Ex el);



	/// \ingroup core
	///
	/// Apply a function on every node in the tree at and below the
	/// given node, depth-first. Return an iterator to the top node,
	/// which replaces 'it' (may be the same).

	template<typename T>
	typename T::iterator do_subtree(const T& tr, typename T::iterator it, std::function<typename T::iterator(typename T::iterator)> f)
		{
		if(it==tr.end()) return it;

		class T::post_order_iterator walk=it, last=it;
		++last;
		walk.descend_all();

		do {
			auto nxt=walk;
			++nxt;

			bool cpy=false;
			if(walk==it) cpy=true;
			walk = f(walk);
			if(cpy) it=walk;

			walk=nxt;
			}
		while(walk!=last);

		return it;
		}

	// Iterate over the children of 'it' if the node is named 'delim', otherwise only
	// iterate over the single node. Note: this yields iterators, not str_nodes.
	struct split_it
	{
		struct iterator {
			using value_type = Ex::sibling_iterator;
			using difference_type = ptrdiff_t;
			using reference = value_type&;
			using const_reference = const value_type&;
			using pointer = value_type*;
			using const_pointer = const value_type*;
			using iterator_category = std::input_iterator_tag;

			iterator() {}
			iterator(Ex::sibling_iterator it) : it(it) {}

			bool operator == (const iterator& other) { return it == other.it; }
			bool operator != (const iterator& other) { return !(*this == other); }
			reference operator* () { return it; }
			pointer operator-> () { return &it; }
			reference operator ++ () { return ++it; }
			value_type operator ++ (int) { return it++; }


			Ex::sibling_iterator it;
		};

		split_it(Ex::iterator it, const std::string& delim = "")
		{
			if (delim == "" || *it->name == delim) {
				begin_ = it.begin();
				end_ = it.end();
			}
			else {
				begin_ = it;
				end_ = it;
				++end_;
			}
		}

		iterator begin() { return iterator(begin_); }
		iterator end() { return iterator(end_); }

	private:
		Ex::sibling_iterator begin_, end_;
	};

	};
