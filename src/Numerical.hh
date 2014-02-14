
#pragma once

#include "Props.hh"

/// Property indicating that a symbolic object always takes integer values.
/// Optionally takes a range over which it runs, which can be symbolic.

class Integer : public property {
	public:
		virtual ~Integer() {};
		virtual std::string name() const;
		virtual bool parse(exptree&, exptree::iterator, exptree::iterator, keyval_t&);
		virtual void display(std::ostream&) const;
		virtual std::string unnamed_argument() const { return "range"; };
		
		exptree from, to, difference;
};


/// Property indicating that an operator is numerically flat, so that
/// numerical factors in the argument can be taken outside.

class NumericalFlat : virtual public property {
	public:
		virtual std::string name() const;
};
