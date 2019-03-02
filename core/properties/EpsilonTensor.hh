
#pragma once

#include "properties/AntiSymmetric.hh"

namespace cadabra {

	/// \ingroup properties

	class EpsilonTensor : public AntiSymmetric, virtual public property {
		public:
			virtual ~EpsilonTensor() {};
			virtual bool parse(Kernel&, keyval_t&) override;
			virtual std::string name() const override;

			Ex metric, krdelta;
		};

	}
