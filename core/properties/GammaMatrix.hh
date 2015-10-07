
#pragma once

#include "properties/Matrix.hh"
#include "properties/AntiSymmetric.hh"

class GammaMatrix : public AntiSymmetric, public Matrix, virtual public property {
	public:
		virtual std::string name() const override;
		virtual void        latex(std::ostream&) const;
		virtual bool        parse(const Properties&, keyval_t& keyvals);
		Ex metric;
};

