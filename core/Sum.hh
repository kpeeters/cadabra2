#pragma once

#include "Storage.hh"
#include "Kernel.hh"
#include "ReservedNode.hh"

namespace cadabra {

	namespace visit {
	
		class Sum : public cadabra::visit::ReservedNode {
			public:
				Sum(const Kernel&, Ex&, Ex::iterator);

				/// Find all terms in the sum which contain the given sub-expression.
				/// Returns iterators pointing to terms.
				std::vector<Ex::iterator> find_terms_containing(Ex::iterator) const;
		};
	};
};
        
