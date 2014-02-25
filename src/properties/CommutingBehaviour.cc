
#include "properties/CommutingBehaviour.hh"

property::match_t CommutingBehaviour::equals(const property *) const
	{
	return no_match; // you can have as many of these as you like
	}

