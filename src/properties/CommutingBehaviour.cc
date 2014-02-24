
#include "properties/CommutingBehaviour.hh"

property_base::match_t CommutingBehaviour::equals(const property_base *) const
	{
	return no_match; // you can have as many of these as you like
	}

