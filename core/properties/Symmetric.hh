
#pragma once

#include "properties/TableauBase.hh"

namespace cadabra {

	class Symmetric : public TableauBase, virtual public property  {
		public:
			virtual std::string  name() const override;
			virtual unsigned int size(const Properties&, Ex&, Ex::iterator) const override;
			virtual tab_t        get_tab(const Properties&, Ex&, Ex::iterator, unsigned int) const override;
	};

}
