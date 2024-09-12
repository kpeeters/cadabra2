
#pragma once

#include "properties/AntiSymmetric.hh"

namespace cadabra {

	/// \ingroup properties

	class EpsilonTensor : public AntiSymmetric, virtual public property {
		public:
			virtual ~EpsilonTensor();
			virtual std::string name() const override;
			virtual bool parse(Kernel&, keyval_t&) override;

			Ex metric, krdelta;
		};

	}
