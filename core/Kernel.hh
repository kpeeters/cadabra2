
#pragma once

#include <functional>
#include "Props.hh"

/// \ingroup core
///
/// Cadabra kernel that keeps all state information that needs to be passed
/// around to algorithms and properties. Stores property information and
/// global settings.

namespace cadabra {

	class Kernel {
		public:
			Kernel(bool inject_defaults=true);
			Kernel(const Kernel& other) = delete;
			~Kernel();

			/// Inject a property into the system and attach it to the given pattern.
			void inject_property(property *prop, std::shared_ptr<Ex> pattern, std::shared_ptr<Ex> property_arguments);

			/// Create an Ex expression object from a string, which will be parsed.
			std::shared_ptr<Ex> ex_from_string(const std::string&);

			
			Properties properties;

			/// Settings.
			enum class scalar_backend_t { sympy, mathematica } scalar_backend;
			bool                                               call_embedded_python_functions;


			const static std::string version;
			const static std::string build  ;

			// Warnings 
			enum class warn_t {
				notset   = 0,
				debug    = 10,
				info     = 20,
				warning  = 30,
				error    = 40,
				critical = 50
			};
			warn_t warning_level;
			std::function<void(const std::string&)> warning_callback;
			void warn(const std::string& msg, warn_t level = warn_t::warning) const;
		};

	}
