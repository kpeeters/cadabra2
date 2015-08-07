
#pragma once

#include "properties/TableauSymmetry.hh"
#include "properties/Traceless.hh"

class WeylTensor : public TableauSymmetry, public Traceless, virtual public property {
	public:
		WeylTensor();
		virtual std::string name() const override;
		virtual void        validate(const Properties&, const Ex&) const override;
		virtual void        display(std::ostream&) const override;
};
