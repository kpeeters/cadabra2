
#pragma once

#include "Props.hh"
#include "properties/TableauSymmetry.hh"

class InverseMetric : public TableauSymmetry, virtual public property {
	public:
		InverseMetric();
		virtual std::string name() const override;
		virtual bool        parse(const Properties&, keyval_t&) override;
		virtual void        validate(const Properties&, const Ex&) const override;

		int signature;
};
