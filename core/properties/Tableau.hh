
#pragma once

#include "Props.hh"
#include "properties/ImplicitIndex.hh"

namespace cadabra {

	class Tableau : public ImplicitIndex, virtual public property {
		public:
			virtual ~Tableau() {};
			virtual std::string name() const override;
			virtual bool        parse(Kernel&, keyval_t& keyvals) override;

			int dimension;
		};

	}
