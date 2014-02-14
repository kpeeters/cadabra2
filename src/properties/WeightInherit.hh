
#pragma once

class WeightInherit : virtual public WeightBase {
	public:
		// The following exception class is thrown when 'value' cannot figure out the 
		// weight because a sum contains terms of different weight. 
		class WeightException : public ConsistencyException { 
			public:
				WeightException(const std::string&);
		};

		virtual bool          parse(exptree&, exptree::iterator, exptree::iterator, keyval_t&);
		virtual multiplier_t  value(exptree::iterator, const std::string& forcedlabel) const;
		virtual std::string   unnamed_argument() const { return "type"; };
		virtual std::string   name() const;
		
		enum { multiplicative, additive } combination_type;

		multiplier_t value_self;
};

