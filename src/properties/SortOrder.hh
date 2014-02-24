
#pragma once

#include "Props.hh"

class SortOrder : public list_property {
	public:
		virtual std::string name() const;
		virtual match_t equals(const property_base *) const;
};
