
#pragma once

#include "Props.hh"
#include "properties/Derivative.hh"

class ExteriorDerivative : public Derivative, virtual public property {
	public:
      virtual std::string name() const override;
		
};
