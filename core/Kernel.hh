
#pragma once

#include "Props.hh"

/// \ingroup core
///
/// Cadabra kernel that keeps all state information that needs to be passed
/// around to algorithms and properties. At the moment only stores property
/// information, but could also store global settings and the like at some
/// later stage.

namespace cadabra {

class Kernel {
	public:
		Kernel();
		Kernel(const Kernel& other) = delete;
		~Kernel();

		/// Inject a property into the system and attach it to the given pattern.
		void inject_property(property *prop, std::shared_ptr<Ex> pattern, std::shared_ptr<Ex> property_arguments);

		/// Create an Ex expression object from a string, which will be parsed.
		std::shared_ptr<Ex> ex_from_string(const std::string&);
		
		
		Properties properties;
};

}
