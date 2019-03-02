
#pragma once

#include "Props.hh"

namespace cadabra {

	class SortOrder : public list_property {
		public:
			virtual std::string name() const;
			virtual match_t equals(const property *) const;
		};

	}
