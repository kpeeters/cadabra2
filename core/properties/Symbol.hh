
#pragma once

#include "Props.hh"

class Symbol : public property {
	public:
		virtual std::string name() const;

		static const Symbol *get(const Properties&, Ex::iterator, bool ignore_parent_rel=false);
};
