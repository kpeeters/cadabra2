#pragma once

#include "Storage.hh"
#include "Kernel.hh"
#include "ReservedNode.hh"

namespace cadabra {

	namespace visit {
	
		class Equals : public cadabra::visit::ReservedNode {
			public:
				Equals(const Kernel&, Ex&, Ex::iterator);
				
				/// Left-hand side.
				Ex::iterator lhs() const;
				Ex::iterator rhs() const;
				
				/// Move all terms in an equality to the left-hand side.
				void move_all_to_lhs();
		};
	};
};
        
