
#pragma once

#include "algorithms/eliminate_metric.hh"

namespace cadabra {

	class eliminate_vielbein : public eliminate_converter {
		public:
			eliminate_vielbein(const Kernel&, Ex&, Ex&);

		protected:
			virtual bool is_conversion_object(iterator) const override;
		};

	}
