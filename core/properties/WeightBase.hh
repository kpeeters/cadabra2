
#pragma once

#include "Props.hh"

class WeightBase : virtual public labelled_property {
	public:
		virtual multiplier_t  value(const Properties&, exptree::iterator, const std::string& forcedlabel) const=0;
};

