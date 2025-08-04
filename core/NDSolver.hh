#pragma once

#include "Storage.hh"
#include "Compare.hh"
#include "Exceptions.hh"
#include "NTensor.hh"
#include "NEvaluator.hh"

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
			// Initialise with an Ex containing the ODEs in standard form
			// (that is, only 1st order derivatives on the lhs).
			NDSolver(const Ex&);

			typedef std::vector<double> state_type;

			// Integration function.
			void operator()(const state_type &x, state_type &dxdt, const double t);

			// Observer function.
			void operator()( const state_type &x , double t );

			// Set and check ODE(s).
			void set_ODEs(const Ex&);

			// Set initial values.
			void set_initial_value(const Ex&, double val);

			// Set integration range for the independent variable.
			void set_range(const Ex&, double from, double to);

			// Set a condition which, when satisfied, will stop the integration.
			void set_stop(const Ex&);
			
			// Entry point.
			std::vector<NTensor> integrate();

			// Functions.
			const std::vector<Ex>& functions() const;
			
		private:
			// Exception used by the observer function to terminate
			// iteration in case an event happens.
			
			class EventException : public CadabraException {
				public:
					EventException(std::string reason="");
			};
			
			Ex ODEs, stop;

			// Information extracted from `ODEs`.
			std::vector<Ex>           variables;

			// For each function in the ODEs we have one evaluator.
			std::vector<NEvaluator>   evaluators;

			// For each condition in the `stop` expression we also have one evaluator.
			std::vector<NEvaluator>   stop_lhs_evaluators, stop_rhs_evaluators;
			std::vector<bool>         stop_conditions;

			// Extract from `ODEs` the right-hand side expressions as
			// well as the names of the functions to solve for.
			void extract_from_ODEs();

			// Evaluate the stop condition.
			bool evaluate_stop(const state_type& x, const double t);

			// Storage of the result of the integration.
			std::vector<std::vector<double>> states;
			std::vector<double>              times;
			std::map<Ex, double, tree_exact_less_no_wildcards_obj>  initial_values;
			Ex                               time_variable;
			double                           range_from, range_to;
};
	
};
