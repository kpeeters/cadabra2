
#pragma once

#include "Props.hh"
#include "properties/TableauSymmetry.hh"

namespace cadabra {

	class InverseMetric : public TableauSymmetry, virtual public property {
		public:
			InverseMetric();
			virtual std::string name() const override;
			virtual bool        parse(Kernel&, keyval_t&) override;
			virtual void        validate(const Kernel&, const Ex&) const override;

			int signature;
		};

	}
