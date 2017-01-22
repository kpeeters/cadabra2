
#pragma once

#include "properties/TableauBase.hh"

namespace cadabra {

	class KroneckerDelta : public TableauBase, virtual public property {
		public:
			virtual ~KroneckerDelta() {};
			virtual std::string name() const;
			
			virtual unsigned int size(const Properties&, Ex&, Ex::iterator) const;
			virtual tab_t        get_tab(const Properties &, Ex&, Ex::iterator, unsigned int) const;
	};
	
}
