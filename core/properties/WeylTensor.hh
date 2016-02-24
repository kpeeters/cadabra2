
#pragma once

#include "properties/TableauSymmetry.hh"
#include "properties/Traceless.hh"

class WeylTensor : public TableauSymmetry, public Traceless, virtual public property {
	public:
		WeylTensor();
		virtual std::string name() const override;
		virtual void        validate(const Kernel&, const Ex&) const override;
		virtual void        latex(std::ostream&) const override;
};
