
#pragma once

#include "Props.hh"

class LaTeXForm : virtual public property {
	public:
		virtual std::string name() const;
		virtual bool parse(exptree&, exptree::iterator, exptree::iterator, keyval_t&);
		virtual std::string unnamed_argument() const;
		std::string latex;
};

