
#include "properties/SortOrder.hh"

property::match_t SortOrder::equals(const property *) const
	{
	return no_match; // you can have as many of these as you like
	}

std::string SortOrder::name() const
	{
	return "SortOrder";
	}
