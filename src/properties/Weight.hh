
#pragma once

#include "CoreProps.hh"

class Weight : virtual public WeightBase {
	public: 
		virtual multiplier_t  value(exptree::iterator, const std::string& forcedlabel) const;
		virtual bool          parse(exptree&, exptree::iterator, exptree::iterator, keyval_t&);
		virtual std::string   unnamed_argument() const { return "value"; };
		virtual std::string   name() const;
	private:
		multiplier_t value_;
};

