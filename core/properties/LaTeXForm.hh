
#pragma once

#include "Props.hh"

namespace cadabra {

	class LaTeXForm : virtual public property {
		public:
			virtual std::string name() const override;
			virtual bool parse(Kernel&, keyval_t&) override;
			virtual std::string unnamed_argument() const override;
			
			std::string latex_form() const;
		private:
			std::string latex_;
	};

}
