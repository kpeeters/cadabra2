
#pragma once

#include "Storage.hh"

namespace cadabra {
	
   /// \ingroup core
   ///
   /// Apply a function on every element of a list, or if the iterator
   /// 'it' does not point to a list, only on that single
   /// element. Handles lists wrapped in an \expression node as well.
	
	void do_list(const Ex& tr, Ex::iterator it, std::function<void(Ex::iterator)> f);
	
	/// \ingroup core
	///
	/// Returns an iterator to the first element for which 'f' does not return tr.end().

	Ex::iterator find_in_list(const Ex& tr, Ex::iterator it, std::function<Ex::iterator(Ex::iterator)> f);

   /// \ingroup core
   ///
   /// Ensure that the tree is a list, even if it contains only a single element.
	
	Ex make_list(Ex el);

};
