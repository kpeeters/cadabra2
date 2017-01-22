
#pragma once

#include "properties/Matrix.hh"
#include "properties/AntiSymmetric.hh"

namespace cadabra {

	class GammaMatrix : public AntiSymmetric, public Matrix, virtual public property {
		public:
			virtual std::string name() const override;
			virtual void        latex(std::ostream&) const override;
			virtual bool        parse(const Kernel&, keyval_t& keyvals) override;
			Ex metric;
	};

}

