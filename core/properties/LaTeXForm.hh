
#pragma once

#include "Props.hh"

class LaTeXForm : virtual public property {
	public:
		virtual std::string name() const;
		virtual bool parse(Ex&, Ex::iterator, Ex::iterator, keyval_t&);
		virtual std::string unnamed_argument() const;

		std::string latex_form() const;
	private:
		std::string latex_;
};

