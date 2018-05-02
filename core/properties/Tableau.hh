
#pragma once

#include "Props.hh"

namespace cadabra {

	class Tableau : public property {
		public:
			virtual ~Tableau() {};
			virtual std::string name() const;
	      virtual bool        parse(Kernel&, keyval_t& keyvals) override;
			
			int dimension;
	};

}
