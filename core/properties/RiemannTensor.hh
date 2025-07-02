#pragma once

#include "properties/TableauSymmetry.hh"

namespace cadabra {

	class RiemannTensor : public TableauSymmetry, virtual public property {
		public:
			RiemannTensor();
			virtual std::string name() const override;
			virtual void        validate(Kernel&, std::shared_ptr<Ex>) const override;
//			virtual bool        parse(Kernel&, std::shared_ptr<Ex>, keyval_t&) override;

		};

	}
