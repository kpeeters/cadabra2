#pragma once

#include "Props.hh"

namespace cadabra {

	// Turns a symbol into an integration function, which takes two arguments,
	// the integrand and the coordinate over which to integrate, optionally
	// with a range.

	class Integral : public property {
	public:
		virtual ~Integral() {};

		virtual std::string name() const;
		virtual bool        parse(Kernel&, keyval_t& keyvals) override;
		virtual void        display(std::ostream&) const;
	};

}
