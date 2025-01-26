#pragma once

#include "Storage.hh"
#include "Exceptions.hh"

namespace cadabra {

	/// \ingroup numerical
	///
	/// Functionality to numerically integrate a set of first-order
	/// ODEs. Currently only supports initial value problems.
	///
	/// The ODEs should be stored in the Ex as a list of equations
	/// of the form
	///
	///   \dot{x} = f(t,x,y),  \dot{y} = g(t,x,y), ...
	///
	/// Here '\dot' needs to be a Derivative, with the 'to' parameter
	/// set to the ODE parameter (which can also appear explictly
	/// on the right-hand sides).
	
	class NDSolver {
		public:
			NDSolver();

			typedef std::vector<double> state_type;

			// Integration function.
			void operator()(const state_type &x, state_type &dxdt, const double t);

			// Observer function.
			void operator()( const state_type &x , double t );

			// Set and check ODE(s).
			void set_ODEs(const Ex&);
			
			// Entry point.
			void integrate();
			
		private:
			// Exception used by the observer function to terminate
			// iteration in case an event happens.
			
			class EventException : public CadabraException {
				public:
					EventException(std::string="");
			};
			
			std::vector< state_type > states;
			std::vector< double >     times;

			Ex ODEs;
	};
	
};
