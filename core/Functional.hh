
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

	/// \ingroup
	///
	/// For lists as defined above for 'do_list', return their size (in case you
	/// really need to know the size before iterating over the elements).

	int list_size(const Ex& tr, Ex::iterator it);
	
	/// \ingroup core
	///
   /// Apply a function on every node in the tree at and below the
   /// given node, depth-first. Return an iterator to the top node,
	/// which replaces 'it' (may be the same).
	
	Ex::iterator do_subtree(const Ex& tr, Ex::iterator it, std::function<Ex::iterator(Ex::iterator)> f);

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

};
