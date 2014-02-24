
#include "properties/SortOrder.hh"

property_base::match_t SortOrder::equals(const property_base *) const
	{
	return no_match; // you can have as many of these as you like
	}

std::string SortOrder::name() const
	{
	return "SortOrder";
	}
