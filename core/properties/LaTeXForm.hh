
#pragma once

#include "Props.hh"

namespace cadabra {

	class LaTeXForm : virtual public property {
		public:
			virtual std::string name() const override;
			virtual bool parse(Kernel&, keyval_t&) override;
			virtual std::string unnamed_argument() const override;

			/// Return the LaTeX string which should be used to display
			/// the object to which this property is associated. Will be
			/// interpreted by DisplayTeX because it can contain patterns
			/// (as e.g. in `a{b??}::LaTeXForm("| b?? \rangle")`).
			std::string latex_form() const;
			
			std::vector<Ex> latex;
		};

	}
