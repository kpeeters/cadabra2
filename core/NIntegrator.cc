
#include "NIntegrator.hh"
#include <boost/math/quadrature/gauss_kronrod.hpp>

using namespace cadabra;

NIntegrator::NIntegrator(const Ex& ex)
	: integrand(ex), evaluator(NEvaluator(ex))
	{
	}

void NIntegrator::set_range(const Ex& x, double from_, double to_)
	{
	ivar=x;
	range_from=from_;
	range_to=to_;
	}

std::complex<double> NIntegrator::integrate()
	{
	double abs_error = 1e-12;
	double error;
	
	// Maximum number of iterations
	size_t max_depth = 15;

	auto eval = [this](double x) -> std::complex<double> {
		evaluator.set_variable(ivar, NTensor(x));
		return evaluator.evaluate().at();
		};

	
	// Perform the integration
	std::complex<double> result =
		boost::math::quadrature::gauss_kronrod<double, 15>::integrate(
			eval, range_from, range_to, max_depth, abs_error, &error);

	return result;
	}
