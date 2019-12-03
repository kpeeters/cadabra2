
#pragma once

#include "Props.hh"

namespace cadabra {

	class Traceless : virtual public property {
		public:
			virtual ~Traceless() {};
			virtual std::string name() const override;
			virtual bool        parse(Kernel&, keyval_t&) override;

			std::string index_set_name; // refers to Indices::set_name
		};

	}

