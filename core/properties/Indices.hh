
#pragma once

#include "Props.hh"

class Indices : public list_property {
	public:
		Indices(); //const std::string& parent="");
		virtual bool parse(exptree&, exptree::iterator, exptree::iterator, keyval_t&);
		virtual std::string name() const;
		virtual std::string unnamed_argument() const { return "name"; };
		virtual match_t equals(const property *) const;
		
		std::string set_name, parent_name;
		enum position_t { free, fixed, independent } position_type;
		exptree     values;
};
