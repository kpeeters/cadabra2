#pragma once

#include "Storage.hh"
#include "Kernel.hh"
#include "Algorithm.hh"

namespace cadabra {

	namespace visit {
	
		class ReservedNode : public ExManip {
			public:
				ReservedNode(const Kernel&, Ex&, Ex::iterator);
				
			protected:
				Ex::iterator   top;
		};
		
		class Equals : public ReservedNode {
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
        
