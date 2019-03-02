
#pragma once

#include "Props.hh"

// Turns a symbol into an integration function, which takes two arguments,
// the integrand and the coordinate over which to integrate, optionally
// with a range.

class Integral : public property {
	public:
		virtual ~Integral() {};

		virtual std::string name() const;
		virtual bool        parse(const Properties&, keyval_t& keyvals) override;
		virtual void        display(std::ostream&) const;
	};
