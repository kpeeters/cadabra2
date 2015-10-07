
#pragma once

#include "Props.hh"

class Indices : public list_property {
	public:
		Indices(); //const std::string& parent="");
		virtual bool parse(const Properties&, keyval_t&) override;
		virtual std::string name() const;
		virtual std::string unnamed_argument() const { return "name"; };
		virtual match_t equals(const property *) const;
		
//		virtual void display(std::ostream&) const override; 
		virtual void latex(std::ostream&) const override; 
		
		std::string set_name, parent_name;
		enum position_t { free, fixed, independent } position_type;
		Ex     values;
};
