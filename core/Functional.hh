
#pragma once

#include "Storage.hh"

namespace cadabra {
	
   /// \ingroup core
   ///
   /// Apply the function on every element of a list, or if 'it' does not
   /// point to a list, only on that single element. Handles lists
   /// wrapped in an \expression node as well.
	
	void do_list(const exptree& tr, exptree::iterator it, std::function<void(exptree::iterator)> f);
	
   /// \ingroup core
   ///
   /// Ensure that the tree is a list, even if it contains only a single element.
	
	exptree make_list(exptree el);

};
