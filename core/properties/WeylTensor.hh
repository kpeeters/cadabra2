
#pragma once

#include "properties/TableauSymmetry.hh"
#include "properties/Traceless.hh"

namespace cadabra {

	class WeylTensor : public TableauSymmetry, public Traceless, virtual public property {
		public:
			WeylTensor();
			virtual ~WeylTensor();
			virtual std::string name() const override;
			virtual bool        parse(Kernel&, keyval_t&) override;
			virtual void        validate(const Kernel&, const Ex&) const override;
			virtual void        latex(std::ostream&) const override;
		};

	}
