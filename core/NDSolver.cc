
#include "NDSolver.hh"
#include <boost/numeric/odeint.hpp>

using namespace cadabra;

void NDSolver::operator()(const state_type&x, state_type& dxdt, const double t)
	{
	// Implement ODE
	}

void NDSolver::operator()(const state_type&x, const double t)
	{
	// To stop iteration before the final t-value has been reached,
	// simply throw an EventException here.
	}

void NDSolver::set_ODEs(const Ex& odes)
	{
	
	}

void NDSolver::integrate()
	{
	state_type x;
	x.push_back(0);
	x.push_back(1);

	std::vector<state_type> x_vec;
	std::vector<double>     times;

	size_t steps=0;
	try {
		steps = boost::numeric::odeint::integrate(*this, x, 0.0, 10.0, 0.1, *this);
		}
	catch(const EventException& ex) {
		// Integration has been stopped by the observer function.
		}
	}
