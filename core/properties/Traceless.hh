
#pragma once

#include "Props.hh"

namespace cadabra {

	class Traceless : virtual public property {
		public:
			virtual ~Traceless() {};
			virtual std::string name() const override;
			virtual bool        parse(Kernel&, keyval_t&) override;
			virtual std::string unnamed_argument() const override
				{
				return "indices";
				};

			std::set<std::string> index_set_names; // refers to Indices::set_name
		};

	}

