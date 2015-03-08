
#pragma once

#include "Props.hh"

class ImplicitIndex : virtual public property {
	public:
		virtual bool parse(const Properties&, keyval_t&) override;
		virtual std::string name() const override;
		virtual std::string unnamed_argument() const { return "name"; };
		virtual void display(std::ostream& str) const override;

		std::vector<std::string> set_names;
};

