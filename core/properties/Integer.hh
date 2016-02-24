
#pragma once

#include "Props.hh"

/// \ingroup properties
///
/// Property indicating that a symbolic object always takes integer values.
/// Optionally takes a range over which it runs, which can be symbolic.

class Integer : public property {
	public:
		virtual ~Integer() {};
		virtual std::string name() const override;
		virtual bool        parse(const Kernel&, keyval_t& keyvals) override;
//		virtual bool parse(Ex&, Ex::iterator, Ex::iterator, keyval_t&);
		virtual void display(std::ostream&) const;
		virtual std::string unnamed_argument() const  override { return "range"; };
		
		Ex from, to, difference;
};


