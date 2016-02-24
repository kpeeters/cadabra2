
#pragma once

#include "Props.hh"
#include "properties/TableauSymmetry.hh"

class Metric : public TableauSymmetry, virtual public property {
	public:
		Metric();
		virtual std::string name() const override;
		virtual bool        parse(const Kernel&, keyval_t&) override;
		virtual void        validate(const Kernel&, const Ex&) const override;

		int signature;
};
