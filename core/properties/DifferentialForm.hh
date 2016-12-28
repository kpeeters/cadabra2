
#pragma once

#include "Props.hh"
#include "properties/ImplicitIndex.hh"

class DifferentialForm : public ImplicitIndex, public IndexInherit, virtual public property {
	public:
      virtual std::string name() const override;
		virtual bool parse(const Kernel&, keyval_t&) override;
		
		Ex degree;
};
