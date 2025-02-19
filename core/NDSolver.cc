
#include "NDSolver.hh"
#include "Functional.hh"
#include <boost/numeric/odeint.hpp>

using namespace cadabra;

NDSolver::NDSolver(const Ex& odes)
	: ODEs(odes), time_variable(Ex("t")), range_from(0), range_to(1)
	{
	}

void NDSolver::operator()(const state_type& x, state_type& dxdt, const double t)
	{
	// Evaluate the right-hand sides of the ODEs at the given point in
	// x's and t.

	for(size_t e=0; e<evaluators.size(); ++e) {
		NEvaluator& evaluator = evaluators[e];
		evaluator.set_variable(time_variable, NTensor(t));
		for(size_t i=0; i<x.size(); ++i) {
			evaluator.set_variable(variables[i], NTensor(x[i]));
			}
		dxdt[e] = evaluator.evaluate().at();
		// std::cerr << x[0] << ", " << x[1] << " -> "
		//			 << dxdt[0] << ", " << dxdt[1] << std::endl;
		}
	}

void NDSolver::operator()(const state_type&x, const double t)
	{
	// This is the observer function, which can also be used to stop
	// integration when a condition is met (to stop iteration before
	// the final t-value has been reached, simply throw an
	// EventException here).

	// std::cerr << this << ", t=" << t << ": " << x[0] << ", " << x[1] << std::endl;
	times.push_back(t);
	for(size_t i=0; i<x.size(); ++i) 
		states[i].push_back( x[i] );
	}

void NDSolver::set_ODEs(const Ex& odes)
	{
	ODEs=odes;
	}

void NDSolver::set_initial_value(const Ex& var, double val)
	{
	auto it = initial_values.find(var);
	if(it == initial_values.end()) {
		initial_values[var] = val;
		}
	else {
		it->second = val;
		}
	}

void NDSolver::set_range(const Ex& var, double f, double t)
	{
	time_variable = var;
	range_from = f;
	range_to   = t;
	}

void NDSolver::extract_from_ODEs()
	{
	evaluators.clear();
	
	do_list(ODEs, ODEs.begin(), [this](Ex::iterator it) {
		if(*it->name!="\\equals")
			throw ConsistencyException("NDSolver: found a non-equation.");

		auto lhs = ODEs.begin(it);
		if(*lhs->name!="\\dot")
			throw ConsistencyException("NDSolver: all left-hand sides must be first-order derivatives.");

		auto var = ODEs.begin(lhs);
		variables.push_back(Ex(var));
		// std::cerr << "found variable " << var << std::endl;

		++lhs;
		evaluators.push_back(NEvaluator(Ex(lhs)));
		// std::cerr << "found rhs " << lhs << std::endl;
		
		return true;
		});
	}

std::vector<NTensor> NDSolver::integrate()
	{
	extract_from_ODEs();
	states.resize(variables.size());
	times.clear();
	states.clear();

	// Setup initial values.
	state_type ivs;
	for(size_t i=0; i<variables.size(); ++i) {
		auto it = initial_values.find( variables[i] );
		if(it != initial_values.end()) 
			ivs.push_back(it->second);
		else 
			ivs.push_back(0);
		}

	size_t steps=0;
	try {
		steps = boost::numeric::odeint::integrate(std::ref(*this),
																ivs,    // initial values
																range_from,
																range_to,
																0.01, // initial step size, adjusts itself
																std::ref(*this));
		}
	catch(const EventException& ex) {
		// Integration has been stopped by the observer function.
		}

	// Create a vector of NTensors to return.
	NTensor ts( times );
	std::vector<NTensor> ret;
	ret.push_back(ts);
	for(size_t i=0; i<variables.size(); ++i) {
		NTensor vs( states[i] );
		ret.push_back(vs);
		}

	return ret;
	}
