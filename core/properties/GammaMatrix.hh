
#pragma once

#include "properties/Matrix.hh"
#include "properties/AntiSymmetric.hh"

class GammaMatrix : public AntiSymmetric, public Matrix, virtual public property {
	public:
		virtual std::string name() const;
		virtual void        display(std::ostream&) const;
		virtual bool        parse(exptree&, exptree::iterator pat, exptree::iterator prop, keyval_t& keyvals);
		exptree metric;
};

