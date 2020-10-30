#pragma once

#include "Props.hh"
#include "properties/ImplicitIndex.hh"

namespace cadabra {

	class FilledTableau : public ImplicitIndex, virtual public property {
		public:
			virtual ~FilledTableau() {};
			virtual std::string name() const override;
			virtual bool        parse(Kernel&, keyval_t& keyvals) override;

			int dimension;
		};

	}
