
#pragma once

#include "properties/ImplicitIndex.hh"

namespace cadabra {

	class Matrix : public ImplicitIndex, virtual public property {
		public:
			virtual ~Matrix() {};
			virtual std::string name() const;
		};

	}
