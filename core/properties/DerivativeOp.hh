
#pragma once

#include "Props.hh"

namespace cadabra {
	
	class DerivativeOp : virtual public property {
		public :
			virtual ~DerivativeOp() {};
			virtual std::string name() const override;
			virtual bool        parse(Kernel&, std::shared_ptr<Ex>, keyval_t& keyvals) override;
	};
	
}
