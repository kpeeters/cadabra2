
#pragma once

#include "properties/DependsBase.hh"

namespace cadabra {

	/// \ingroup properties

	class DependsInherit : public DependsBase, virtual public property {
		public:
			virtual std::string name() const override;
			virtual Ex dependencies(const Kernel&, Ex::iterator) const override;
		};

	}
