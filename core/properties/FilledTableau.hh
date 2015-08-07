#pragma once

#include "Props.hh"

class FilledTableau : public property {
	public:
		virtual ~FilledTableau() {};
		virtual std::string name() const;
		virtual bool parse(Ex&, Ex::iterator, Ex::iterator, keyval_t&);

		int dimension;
};

