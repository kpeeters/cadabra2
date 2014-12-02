
#pragma once

#include "properties/WeightBase.hh"

class Weight : virtual public WeightBase {
	public: 
		virtual multiplier_t  value(const Properties&, exptree::iterator, const std::string& forcedlabel) const override;
		virtual bool          parse(exptree&, exptree::iterator, exptree::iterator, keyval_t&);
		virtual std::string   unnamed_argument() const { return "value"; };
		virtual std::string   name() const;
	private:
		multiplier_t value_;
};

