
#pragma once

#include "Props.hh"
#include "properties/Derivative.hh"
#include "properties/DifferentialFormBase.hh"

class ExteriorDerivative : public Derivative, public DifferentialFormBase {
	public:
      virtual std::string name() const override;

		virtual Ex degree(const Properties&, Ex::iterator) const override;
};
