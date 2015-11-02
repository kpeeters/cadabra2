
#pragma once

#include "Props.hh"

class LaTeXForm : virtual public property {
	public:
		virtual std::string name() const override;
		virtual bool parse(const Properties&, keyval_t&) override;
		virtual std::string unnamed_argument() const override;

		std::string latex_form() const;
	private:
		std::string latex_;
};

