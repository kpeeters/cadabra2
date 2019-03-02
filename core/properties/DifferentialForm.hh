
#pragma once

#include "Props.hh"
#include "properties/ImplicitIndex.hh"
#include "properties/IndexInherit.hh"
#include "properties/DifferentialFormBase.hh"

namespace cadabra {

	class DifferentialForm : public IndexInherit, public DifferentialFormBase {
		public:
			virtual std::string name() const override;
			virtual bool parse(Kernel&, keyval_t&) override;

			virtual Ex degree(const Properties&, Ex::iterator) const override;

		private:
			Ex degree_;
		};

	}
