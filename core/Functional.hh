
#pragma once

#include "Storage.hh"

namespace cadabra {
	
   /// \ingroup core
   ///
   /// Apply the function on every element of a list, or if 'it' does not
   /// point to a list, only on that single element. Handles lists
   /// wrapped in an \expression node as well.
	
	void do_list(const Ex& tr, Ex::iterator it, std::function<void(Ex::iterator)> f);
	
   /// \ingroup core
   ///
   /// Ensure that the tree is a list, even if it contains only a single element.
	
	Ex make_list(Ex el);

};
