#pragma once

#include "properties/DependsBase.hh"

namespace cadabra {

	class Depends : public DependsBase, virtual public property {
		public:
			virtual std::string name() const override;
			virtual bool parse(Kernel&, keyval_t&) override;
			virtual Ex dependencies(const Kernel&, Ex::iterator) const override;
			virtual std::string unnamed_argument() const override
				{
				return "dependants";
				};
		private:
			Ex dependencies_;
		};

	}
