
#pragma once

#include "Exceptions.hh"
#include "properties/WeightBase.hh"


class WeightInherit : virtual public WeightBase {
	public:
		// The following exception class is thrown when 'value' cannot figure out the 
		// weight because a sum contains terms of different weight. 
		class WeightException : public ConsistencyException { 
			public:
				WeightException(const std::string&);
		};

		virtual bool          parse(const Properties&, keyval_t&) override;
		virtual multiplier_t  value(const Properties&, Ex::iterator, const std::string& forcedlabel) const override;
		virtual std::string   unnamed_argument() const override { return "type"; };
		virtual std::string   name() const override;
		
		enum { multiplicative, additive } combination_type;

		multiplier_t value_self;
};

