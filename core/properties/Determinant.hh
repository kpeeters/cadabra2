
#pragma once

#include "Props.hh"
#include "properties/Symmetric.hh"

namespace cadabra {

	class Determinant : virtual public property {
		public:
			Determinant();
			virtual std::string name() const override;
			virtual std::string unnamed_argument() const override;
			virtual bool        parse(Kernel&, keyval_t&) override;
			virtual void        validate(const Kernel&, const Ex&) const override;
			virtual void        latex(std::ostream&) const override;
			
			Ex obj;
	};

}
